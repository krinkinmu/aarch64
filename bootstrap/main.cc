#include "util/stdint.h"
#include "util/string.h"
#include "util/utility.h"
#include "util/intrusive_list.h"
#include "util/new.h"
#include "util/vector.h"
#include "util/allocator.h"
#include "fdt/blob.h"
#include "memory/cache.h"
#include "memory/memory.h"
#include "memory/space.h"
#include "bootstrap/memory.h"
#include "bootstrap/log.h"


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
        if (util::StringView("dtb") == data[i].name) {
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
    Log() << "Memory Map:\n";
    for (auto it = mmap.ConstBegin(); it != mmap.ConstEnd(); ++it) {
        Log() << "[" << reinterpret_cast<const void*>(it->begin)
              << "-" << reinterpret_cast<const void*>(it->end)
              << ") ";
        if (it->status == memory::MemoryStatus::RESERVED) {
            Log() << "reserved\n";
        } else {
            Log() << "free\n";
        }
    }
}

[[ noreturn ]] void Panic() {
    while (1) {
        asm volatile("":::"memory");
    }    
}

struct Item : public util::ListNode<Item> {
    memory::Contigous m;
};

void AllocatorTest() {
    constexpr size_t kSize = 4096;
    util::IntrusiveList<Item> items;
    size_t allocated = 0;

    while (1) {
        auto m = memory::AllocatePhysical(kSize);
        if (!m.Size()) {
            break;
        }
        util::memset(reinterpret_cast<void*>(m.FromAddress()), 0, m.Size());
        Item* item = reinterpret_cast<Item*>(m.FromAddress());
        ::new(static_cast<void*>(&item->m)) memory::Contigous(util::Move(m));
        items.LinkAt(items.Begin(), item);
        ++allocated;
    }

    Log() << "Allocated " << allocated << " " << kSize << " byte pages\n";

    size_t freed = 0;

    while (!items.Empty()) {
        Item* item = &*items.Begin();
        items.PopFront();
        memory::FreePhysical(util::Move(item->m));
        ++freed;
    }

    Log() << "Freed " << freed << " " << kSize << " byte pages\n";
}

struct Pointer : public util::ListNode<Pointer> {
    char buf[512];
    void* ptr;
};

void CacheTest() {
    memory::Cache cache(sizeof(Pointer), alignof(Pointer));
    util::IntrusiveList<Pointer> ptrs;
    size_t allocated = 0;

    while (1) {
        void* p = cache.Allocate();
        if (p == nullptr) {
            break;
        }
        util::memset(p, 0, sizeof(Pointer));
        Pointer* ptr = reinterpret_cast<Pointer*>(p);
        ::new(ptr) Pointer();
        ptr->ptr = p;
        ptrs.LinkAt(ptrs.Begin(), ptr);
        ++allocated;
    }

    Log() << "Allocated " << allocated
          << " items of size " << sizeof(Pointer)
          << " bytes and with alignment of " << alignof(Pointer) << " bytes\n";
    Log() << "Available " << memory::AvailablePhysical() << " bytes\n";
    size_t freed = 0;

    while (!ptrs.Empty()) {
        Pointer* ptr = &*ptrs.Begin();
        ptrs.PopFront();
        cache.Free(ptr->ptr);
        freed++;
    }

    Log() << "Freed " << allocated
          << " items of size " << sizeof(Pointer)
          << " bytes and with alignment of " << alignof(Pointer) << " bytes\n";
    Log() << "Available " << memory::AvailablePhysical()
          << " bytes after freeing\n";

    cache.Reclaim();

    Log() << "Available " << memory::AvailablePhysical()
          << " bytes after reclaim\n";
}

struct LargeItem {
    char buf[128];

    LargeItem() {}
    LargeItem(const LargeItem& other) = default;
    LargeItem(LargeItem&& other) = default;
};

void VectorTest() {
    Log() << "Available " << memory::AvailablePhysical()
          << " bytes before vector test\n";

    {
        util::Vector<LargeItem, util::PhysicalAllocator<LargeItem>> v;

        while (v.PushBack(LargeItem())) {
            if ((v.Size() % 100000) == 0) {
                Log() << "Current vector size " << v.Size() << "\n";
            }
        }

        Log() << "Vector size " << v.Size() << " entries currently\n";
        Log() << "Available " << memory::AvailablePhysical()
              << " bytes after filling vector\n";
    }

    Log() << "Available " << memory::AvailablePhysical()
          << " bytes after deleting vector\n";
}

}  // namespace

extern "C" void kernel(struct Data *data, size_t size) {
    SetupLogger();

    uint64_t dtb_begin, dtb_end;

    Log() << "Looking up the DTB...\n";
    if (!LookupDTB(data, size, &dtb_begin, &dtb_end)) {
        Log() << "Failed to lookup the DTB\n";
        Panic();
    }

    Log() << "Setting up DTB Parser...\n";
    fdt::Blob blob;
    if (!fdt::Blob::Parse(
            reinterpret_cast<const uint8_t*>(dtb_begin),
            dtb_end - dtb_begin,
            &blob)) {
        Log() << "Failed to setup DTB parser!\n";
        Panic();
    }

    Log() << "Initializaing memory map...\n";
    static memory::MemoryMap mmap;
    if (!MMapFromDTB(blob, &mmap)) {
        Log() << "Failed to initialize memory map!\n";
        Panic();
    }

    Log() << "Reserve used memory in the memory map...\n";
    if (!ReserveMemory(data, size, &mmap)) {
        Log() << "Failed to reserved used memory in the memory map!\n";
        Panic();
    }
    PrintMMap(mmap);

    Log() << "Initializing memory allocator...\n";
    if (!memory::SetupAllocator(&mmap)) {
        Log() << "Failed to initialize memory allocator!\n";
        Panic();
    }

/*
    Log() << "Preparing page tables...\n";
    memory::AddressSpace aspace;
    if (!memory::SetupAddressSpace(mmap, &aspace)) {
        Log() << "Failed to prepare page tables!\n";
        Panic();
    }

    Log() << "Installing page tables...\n";
    if (!memory::SetupMapping(aspace)) {
        Log() << "Failed to install page tables!\n";
        Panic();
    }
*/

    Log() << "Initialization complete.\n";
    Log() << "Total " << memory::TotalPhysical() << " bytes\n";
    Log() << "Available " << memory::AvailablePhysical() << " bytes\n";

    AllocatorTest();
    AllocatorTest();
    AllocatorTest();

    Log() << "Available after test " << memory::AvailablePhysical() << " bytes\n";

    CacheTest();
    CacheTest();
    CacheTest();

    Log() << "Available after test " << memory::AvailablePhysical() << " bytes\n";

    VectorTest();
    VectorTest();
    VectorTest();

    Log() << "Finished.";

    Panic();
}
