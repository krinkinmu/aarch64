#include <cstdint>
#include <cstring>

#include <new>
#include <utility>

#include "common/intrusive_list.h"
#include "common/string_view.h"
#include "util/vector.h"
#include "util/allocator.h"
#include "fdt/blob.h"
#include "memory/cache.h"
#include "memory/memory.h"
#include "bootstrap/memory.h"
#include "bootstrap/pl011.h"
#include "common/logging.h"

namespace {

// We get a list of these from the UEFI bootloader. It describes modules that
// the bootloader loaded in memory for us. It's up to the bootstrap code to
// convert this list into struct boot_info below that the kernel expects.
struct Data {
    const char *name;
    uint64_t begin;
    uint64_t end;
};

bool LookupDTB(
        const struct Data *data, size_t size, uint64_t *begin, uint64_t *end) {
    for (size_t i = 0; i < size; ++i) {
        if (common::StringView("dtb") == data[i].name) {
            *begin = data[i].begin;
            *end = data[i].end;
            return true;
        }
    }
    return false;
}

bool ReserveMemory(
        const struct Data *data, size_t size, memory::MemoryMap* mmap) {
    for (size_t i = 0; i < size; ++i) {
        if (!mmap->Reserve(data[i].begin, data[i].end)) {
            return false;
        }
    }
    return true;
}

void PrintMMap(const memory::MemoryMap& mmap) {
    common::Log() << "Memory Map:\n";
    for (auto it = mmap.ConstBegin(); it != mmap.ConstEnd(); ++it) {
        common::Log() << "[" << reinterpret_cast<const void*>(it->begin)
              << "-" << reinterpret_cast<const void*>(it->end)
              << ") ";
        if (it->status == memory::MemoryStatus::RESERVED) {
            common::Log() << "reserved\n";
        } else {
            common::Log() << "free\n";
        }
    }
}

[[ noreturn ]] void Panic() {
    while (1) {
        asm volatile("":::"memory");
    }    
}

struct Item : public common::ListNode<Item> {
    memory::Contigous m;
};

void UniquePtrTest() {
    constexpr size_t kSize = 4096;

    for (size_t i = 0; i != 10; ++i) {
        common::Log() << "Available " << memory::AvailablePhysical() << " bytes before allocation\n";
        {
            auto m = memory::AllocatePhysical(kSize);
            if (!m) {
                common::Log() << "Failed to allocate " << kSize << " bytes\n";
                break;
            }
            memset(reinterpret_cast<void*>(m->FromAddress()), 0, m->Size());
            common::Log() << "Available " << memory::AvailablePhysical() << " bytes after allocation\n";
        }
        common::Log() << "Available " << memory::AvailablePhysical() << " bytes after free\n";
    }
}

void AllocatorTest() {
    constexpr size_t kSize = 4096;
    common::IntrusiveList<Item> items;
    size_t allocated = 0;

    while (1) {
        auto m = memory::AllocatePhysical(kSize).release();
        if (!m.Size()) {
            break;
        }
        memset(reinterpret_cast<void*>(m.FromAddress()), 0, m.Size());
        Item* item = reinterpret_cast<Item*>(m.FromAddress());
        ::new(static_cast<void*>(&item->m)) memory::Contigous(m);
        items.LinkAt(items.Begin(), item);
        ++allocated;
    }

    common::Log() << "Allocated " << allocated << " " << kSize << " byte pages\n";

    size_t freed = 0;

    while (!items.Empty()) {
        Item* item = items.PopFront();
        memory::FreePhysical(item->m);
        ++freed;
    }

    common::Log() << "Freed " << freed << " " << kSize << " byte pages\n";
}

struct Pointer : public common::ListNode<Pointer> {
    char buf[512];
    void* ptr;
};

void CacheTest() {
    memory::Cache cache(sizeof(Pointer), alignof(Pointer));
    common::IntrusiveList<Pointer> ptrs;
    size_t allocated = 0;

    while (1) {
        void* p = cache.Allocate();
        if (p == nullptr) {
            break;
        }
        memset(p, 0, sizeof(Pointer));
        Pointer* ptr = reinterpret_cast<Pointer*>(p);
        ::new(ptr) Pointer();
        ptr->ptr = p;
        ptrs.LinkAt(ptrs.Begin(), ptr);
        ++allocated;
    }

    common::Log() << "Allocated " << allocated
          << " items of size " << sizeof(Pointer)
          << " bytes and with alignment of " << alignof(Pointer) << " bytes\n";
    common::Log() << "Available " << memory::AvailablePhysical() << " bytes\n";
    size_t freed = 0;

    while (!ptrs.Empty()) {
        Pointer* ptr = ptrs.PopFront();
        cache.Free(ptr->ptr);
        freed++;
    }

    common::Log() << "Freed " << allocated
          << " items of size " << sizeof(Pointer)
          << " bytes and with alignment of " << alignof(Pointer) << " bytes\n";
    common::Log() << "Available " << memory::AvailablePhysical()
          << " bytes after freeing\n";

    cache.Reclaim();

    common::Log() << "Available " << memory::AvailablePhysical()
          << " bytes after reclaim\n";
}

struct LargeItem {
    char buf[128];

    LargeItem() {}
    LargeItem(const LargeItem& other) = default;
    LargeItem(LargeItem&& other) = default;
};

void VectorTest() {
    common::Log() << "Available " << memory::AvailablePhysical()
          << " bytes before vector test\n";

    {
        util::Vector<LargeItem, util::PhysicalAllocator<LargeItem>> v;

        while (v.PushBack(LargeItem())) {
            if ((v.Size() % 100000) == 0) {
                common::Log() << "Current vector size " << v.Size() << "\n";
            }
        }

        common::Log() << "Vector size " << v.Size() << " entries currently\n";
        common::Log() << "Available " << memory::AvailablePhysical()
              << " bytes after filling vector\n";
    }

    common::Log() << "Available " << memory::AvailablePhysical()
          << " bytes after deleting vector\n";
}

void SetupLogger() {
    // For HiKey960 board that I have the following parameters were found to        // work fine:
    //
    // PL011::Serial(
    //     /* base_address = */0xfff32000, /* base_clock = */19200000);
    static PL011 serial = PL011::Serial(0x9000000, 24000000);
    static PL011OutputStream stream(&serial);
    common::RegisterLog(&stream); 
}

}  // namespace

extern "C" void kernel(struct Data *data, size_t size) {
    SetupLogger();

    uint64_t dtb_begin, dtb_end;

    common::Log() << "Looking up the DTB...\n";
    if (!LookupDTB(data, size, &dtb_begin, &dtb_end)) {
        common::Log() << "Failed to lookup the DTB\n";
        Panic();
    }

    common::Log() << "Setting up DTB Parser...\n";
    fdt::Blob blob;
    if (!fdt::Blob::Parse(
            reinterpret_cast<const uint8_t*>(dtb_begin),
            dtb_end - dtb_begin,
            &blob)) {
        common::Log() << "Failed to setup DTB parser!\n";
        Panic();
    }

    common::Log() << "Initializaing memory map...\n";
    static memory::MemoryMap mmap;
    if (!MMapFromDTB(blob, &mmap)) {
        common::Log() << "Failed to initialize memory map!\n";
        Panic();
    }

    common::Log() << "Reserve used memory in the memory map...\n";
    if (!ReserveMemory(data, size, &mmap)) {
        common::Log() << "Failed to reserved used memory in the memory map!\n";
        Panic();
    }
    PrintMMap(mmap);

    common::Log() << "Initializing memory allocator...\n";
    if (!memory::SetupAllocator(&mmap)) {
        common::Log() << "Failed to initialize memory allocator!\n";
        Panic();
    }

/*
    common::Log() << "Preparing page tables...\n";
    memory::AddressSpace aspace;
    if (!memory::SetupAddressSpace(mmap, &aspace)) {
        common::Log() << "Failed to prepare page tables!\n";
        Panic();
    }

    common::Log() << "Installing page tables...\n";
    if (!memory::SetupMapping(aspace)) {
        common::Log() << "Failed to install page tables!\n";
        Panic();
    }
*/

    common::Log() << "Initialization complete.\n";
    common::Log() << "Total " << memory::TotalPhysical() << " bytes\n";
    common::Log() << "Available " << memory::AvailablePhysical() << " bytes\n";

    UniquePtrTest();
    UniquePtrTest();
    UniquePtrTest();

    common::Log() << "Available after test " << memory::AvailablePhysical() << " bytes\n";

    AllocatorTest();
    AllocatorTest();
    AllocatorTest();

    common::Log() << "Available after test " << memory::AvailablePhysical() << " bytes\n";

    CacheTest();
    CacheTest();
    CacheTest();

    common::Log() << "Available after test " << memory::AvailablePhysical() << " bytes\n";

    VectorTest();
    VectorTest();
    VectorTest();

    common::Log() << "Finished.";

    Panic();
}
