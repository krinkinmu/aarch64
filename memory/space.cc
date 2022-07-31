#include "memory/space.h"
#include "memory/arch.h"
#include "memory/memory.h"
#include "util/utility.h"
#include "util/string.h"
#include "util/stdint.h"
#include "util/stddef.h"
#include "util/math.h"

namespace memory {

namespace {

constexpr uint64_t kPresent = 1ull << 0;
constexpr uint64_t kTable = 1ull << 1;
// We configure MAIR_EL2 so that only 2 entries there are valid: one for
// normal memory and one for the device memory. For device memory we
// configure full nGnRnE and for normal memory we configure Write-back
// caching policy with non-transient allocation on reads and writes for both
// outer and inner domains.
//
// We use entry MAIR_EL2 entry 1 for the normal memory and MAIR_EL2 entry 2 for
// the device memory.
constexpr uint64_t kNormalMemory = 1ull << 2;
constexpr uint64_t kDeviceMemory = 2ull << 2;
constexpr uint64_t kMemoryAttributeMask = 7ull << 2;

constexpr uint64_t kPrivileged = 1ull << 6;
constexpr uint64_t kWritable = 1ull << 7;
constexpr uint64_t kExecuteNever = 1ull << 54;
constexpr uint64_t kAccessMask = kPrivileged | kWritable | kExecuteNever;

constexpr uint64_t kAttributesMask = kAccessMask | kMemoryAttributeMask;
constexpr uint64_t kAddressMask = ((1ull << 48) - 1) & ~((1ull << 12) - 1);

constexpr size_t kPageSize = 4096; 
constexpr size_t kLowBit[] = { 39, 30, 21, 12 };
constexpr size_t kHighBit[] = { 47, 38, 29, 20 };

}  // namespace

struct PageTable {
    static constexpr size_t kPageTableSize = 512;

    uint64_t descriptors[kPageTableSize];
    Contigous memory;
    AddressSpace* address_space = nullptr;

    PageTable(Contigous memory)
            : memory(util::Move(memory)) {
        util::memset(descriptors, 0, sizeof(descriptors));
    }

    PageTable(const PageTable& other) = delete;
    PageTable& operator=(const PageTable& other) = delete;
    PageTable(PageTable&& other) = delete;
    PageTable& operator=(PageTable&& other) = delete;

    uintptr_t Address() const {
        return memory.FromAddress();
    }

    AddressSpace* AddressSpace() const {
        return address_space;
    }

    void Clear(size_t entry) {
        descriptors[entry] = 0;
    }

    void SetTable(size_t entry, PageTable* child) {
        descriptors[entry] = child->Address() | kPresent | kTable;
    }

    PageTable* Table(size_t entry) const {
        if (!IsTable(entry)) {
            return nullptr;
        }

        const uintptr_t addr = descriptors[entry] & kAddressMask;
        return reinterpret_cast<PageTable*>(addr);
    }

    bool IsTable(size_t entry) const {
        const uint64_t mask = kPresent | kTable;

        return (descriptors[entry] & mask) == mask;
    }

    void SetMemory(size_t entry, uintptr_t addr, uint64_t attr) {
        descriptors[entry] = addr | attr | kPresent;
    }

    uintptr_t MemoryAddress(size_t entry) const {
        if (!IsMemory(entry)) {
            return 0;
        }
        return descriptors[entry] & kAddressMask;
    }

    uint64_t MemoryAttributes(size_t entry) const {
        if (!IsMemory(entry)) {
            return 0;
        }
        return descriptors[entry] & kAttributesMask;
    }

    bool IsMemory(size_t entry) const {
        const uint64_t mask = kPresent | kTable;

        return (descriptors[entry] & mask) == kPresent;
    }
};

struct Request {
    uintptr_t virt_begin;
    uintptr_t virt_end;
    uintptr_t phys_begin;
    uintptr_t phys_end;
    uint64_t attributes;
};

struct Context {
    PageTable* parent;
    size_t entry;
    size_t level;
    uintptr_t entry_begin;
    uintptr_t entry_end;
};

namespace {

size_t EntrySize(size_t level) {
    constexpr size_t kEntrySize[] = {
        1ull << kLowBit[0],
        1ull << kLowBit[1],
        1ull << kLowBit[2],
        1ull << kLowBit[3],
    };
    return kEntrySize[level];
}

size_t Entry(uintptr_t addr, size_t level) {
    return util::Bits(addr, kLowBit[level], kHighBit[level]) >> kLowBit[level];
}

uintptr_t AlignDownToEntry(uintptr_t addr, size_t level) {
    return addr & ~((1ull << kLowBit[level]) - 1);
}

bool CoAligned(uintptr_t virt, uintptr_t phys, uintptr_t alignment) {
    return util::AlignDown(virt, alignment) == virt
        && util::AlignDown(phys, alignment) == phys;
}

bool CanMapDirectly(const Request& req, const Context& ctx) {
    if (ctx.level == 0) {
        return false;
    }
    if (ctx.entry_begin != req.virt_begin) {
        return false;
    }
    if (ctx.entry_end != req.virt_end) {
        return false;
    }
    return CoAligned(req.virt_begin, req.phys_begin, EntrySize(ctx.level));
}

bool CompatibleMapping(const Request& req, const Context& ctx) {
    return (ctx.parent->MemoryAddress(ctx.entry) == req.phys_begin)
        && (ctx.parent->MemoryAttributes(ctx.entry) == req.attributes);
}

}  // namespace


PageTable* AddressSpace::AllocatePageTable() {
    Contigous memory = AllocatePhysical(sizeof(PageTable)).release();
    if (memory.Size() == 0) {
        return nullptr;
    }
    PageTable* table = reinterpret_cast<PageTable*>(memory.FromAddress());
    ::new (table) PageTable(memory);
    table->address_space = this;
    return table;
}

void AddressSpace::FreePageTable(PageTable* table) {
    if (table == nullptr) {
        return;
    }
    Contigous memory = table->memory;
    table->~PageTable();
    FreePhysical(memory);
}

void AddressSpace::Clear(PageTable* table) {
    for (size_t entry = 0; entry < PageTable::kPageTableSize; ++entry) {
        PageTable* child = table->Table(entry);
        if (child == nullptr) {
            continue;
        }
        table->Clear(entry);
        Clear(child);
    }
    FreePageTable(table);
}

AddressSpace::AddressSpace() : root_(nullptr) {}

AddressSpace::~AddressSpace() {
    if (root_ == nullptr) {
        return;
    }
    Clear(root_);
    root_ = nullptr;
}

uintptr_t AddressSpace::Base() const {
    if (root_ != nullptr) {
        return root_->Address();
    }
    return 0;
}

bool AddressSpace::Map(
        uintptr_t phys, uintptr_t virt, size_t size, AccessMode mode) {
    uint64_t attr = 0;

    switch (mode) {
    case AccessMode::ReadOnly:
        attr = kPrivileged | kNormalMemory;
        break;
    case AccessMode::ReadWrite:
        attr = kWritable | kPrivileged | kNormalMemory;
        break;
    case AccessMode::Executable:
        attr = kPrivileged | kNormalMemory;
        break;
    }
    return MapInternal(phys, virt, size, attr);
}

bool AddressSpace::IoMap(
        uintptr_t phys, uintptr_t virt, size_t size, AccessMode mode) {
    uint64_t attr = 0;

    switch (mode) {
    case AccessMode::ReadOnly:
        attr = kPrivileged | kDeviceMemory;
        break;
    case AccessMode::ReadWrite:
        attr = kWritable | kPrivileged | kDeviceMemory;
        break;
    case AccessMode::Executable:
        attr = kPrivileged | kNormalMemory;
        break;
    }
    return MapInternal(phys, virt, size, attr);
}

bool AddressSpace::MapInternal(
        uintptr_t phys, uintptr_t virt, size_t size, uint64_t attr) {
    if (util::AlignDown(phys, kPageSize) != phys) {
        return false;
    }

    if (util::AlignDown(virt, kPageSize) != virt) {
        return false;
    }

    if (util::AlignDown(size, kPageSize) != size) {
        return false;
    }

    if (root_ == nullptr) {
        root_ = AllocatePageTable();
        if (root_ == nullptr) {
            return false;
        }
    }

    Request request;
    request.virt_begin = virt;
    request.virt_end = virt + size;
    request.phys_begin = phys;
    request.phys_end = phys + size;
    request.attributes = attr;
    return MapTableEntries(request, root_, 0);
}

bool AddressSpace::MapTableEntries(
        const Request& req, PageTable* table, size_t level) {
    const uintptr_t from = Entry(req.virt_begin, level);
    const uintptr_t to = Entry(req.virt_end - 1, level);
    const size_t size = EntrySize(level);

    uintptr_t entry_begin = AlignDownToEntry(req.virt_begin, level);
    Context ctx;
    ctx.parent = table;
    ctx.level = level;
 
    for (size_t entry = from; entry <= to; ++entry, entry_begin += size) {
        ctx.entry = entry;
        ctx.entry_begin = entry_begin;
        ctx.entry_end = entry_begin + size;

        Request copy = req;

        if (copy.virt_begin < ctx.entry_begin) {
            const size_t diff = ctx.entry_begin - copy.virt_begin;
            copy.phys_begin += diff;
            copy.virt_begin = ctx.entry_begin;
        }

        if (copy.virt_end > ctx.entry_end) {
            const size_t diff = req.virt_end - ctx.entry_end;
            copy.phys_end -= diff;
            copy.virt_end = ctx.entry_end;
        }

        if (!MapTableEntry(copy, ctx)) {
            return false;
        }
    }

    return true;
}

bool AddressSpace::MapTableEntry(const Request& req, const Context& ctx) {
    if (ctx.parent->IsMemory(ctx.entry)) {
        return CompatibleMapping(req, ctx);
    }

    if (!ctx.parent->IsTable(ctx.entry) && CanMapDirectly(req, ctx)) {
        ctx.parent->SetMemory(ctx.entry, req.phys_begin, req.attributes);
        return true;
    }

    if (!ctx.parent->IsTable(ctx.entry)) {
        PageTable* child = AllocatePageTable();
        if (child == nullptr) {
            return false;
        }
        ctx.parent->SetTable(ctx.entry, child);
    }

    PageTable* table = ctx.parent->Table(ctx.entry);
    return MapTableEntries(req, table, ctx.level + 1);
}

bool SetupAddressSpace(const MemoryMap& mmap, AddressSpace* space) {
   return true; 
}

bool SetupMapping(const AddressSpace& space) {
    if (space.Base() == 0) {
        return false;
    }

    // AArch64 System Register Descriptions, D13.2 General system control
    // registers, D13.2.85 MAIR_EL2, Memory Attribute Indirection Register (EL2)
    // for the encoding of the MAIR_EL2.
    constexpr uint64_t mair = ((0xffull) << 8) | (0x0ull << 16);

    SetMairEl2(mair);
    SetTtbar0El2(space.Base());
    return true;
}

}  // namespace memory
