#ifndef __MEMORY_SPACE_H__
#define __MEMORY_SPACE_H__

#include <cstddef>
#include <cstdint>


namespace memory {

namespace impl {

struct Memory {
    uintptr_t addr;
    uint64_t attr;
};

class PageTable {
public:
    static constexpr size_t kPageTableSize = 512;
    static constexpr uint64_t kPresent = 1ull << 0;
    static constexpr uint64_t kTable = 1ull << 1;

    // These flags need to match MAIR_EL2 register configuration.
    // I only have normal memory configured in MAIR_EL2 for now (write-back
    // caching policy, non-transient allocation on reads and writes for both
    // outer and inner domains).
    static constexpr uint64_t kNormalMemory = 1ull << 2;
    static constexpr uint64_t kMemoryAttributeMask = kNormalMemory;

    static constexpr uint64_t kPrivileged = 1ull << 6;
    static constexpr uint64_t kWritable = 1ull << 7;
    static constexpr uint64_t kExecuteNever = 1ull << 54;
    static constexpr uint64_t kAccessMask = kPrivileged | kWritable | kExecuteNever;

    static constexpr uint64_t kAttributesMask = kAccessMask | kMemoryAttributeMask;
    static constexpr uint64_t ((1ull << 48) - 1) & ~((1ull << 12) - 1);

    PageTable(uintptr_t address);

    PageTable(const PageTable& other) = delete;
    PageTable& operator=(const PageTable& other) = delete;

    PageTable(PageTable&& other) = default;
    PageTable& operator=(PageTable&& other) = default;

    uintptr_t Address() const;

    bool IsClear(size_t entry) const;
    bool IsTable(size_t entry) const;
    bool IsMemory(size_t entry) const;

    void Clear(size_t entry);
    void SetTable(size_t entry, const PageTable& table);
    void SetMemory(size_t entry, const Memory& memory);

    Memory GetMemory(size_t entry) const;
    PageTable GetTable(size_t entry) const;

private:
    uint64_t *descriptors_;
};

}  // namespace impl


struct MemoryRange {
    uintptr_t from;
    uintptr_t to;

    bool Overlap(const MemoryRange& other) const;
    bool Touch(const MemoryRange& other) const;
    bool Before(const MemoryRange& other) const;
    bool After(const MemoryRange& other) const;
};

class Mapping {
public:
    Mapping(const MemoryRange& range, uint64_t attrs);
    virtual ~Mapping() {}

    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;
    Mapping(Mapping&&) = delete;
    Mapping& operator=(Mapping&&) = delete;

    MemoryRange AddressRange() const;
    uint64_t Attributes() const;

    virtual bool Map(uintptr_t from, uintptr_t to, PageTable* root) = 0;
    virtual void Unmap(uintptr_t from, uintptr_t to, PageTable* root) = 0;
    virtual bool Fault(uintptr_t addr, PageTable* root) = 0;

private:
    MemoryRange range_;
    uint64_t attrs_;
};

class StaticMapping : public Mapping {
public:
    MemoryMapping(const MemoryRange& virt, const MemoryRange& phys, uint64_t attrs);

    bool Map(uintptr_t from, uintptr_t to, PageTable* root) override;
    void Unmap(uintptr_t from, uintptr_t to, PageTable* root) override;
    bool Fault(uintptr_t addr, PageTable* root) override;

private:
    MemoryRange phys_;
};

class AddressSpace {
public:
    uintptr_t PageTable() const;

    bool RegisterMapping(std::unique_ptr<Mapping> mapping);
    const Mapping* FindMapping(uintptr_t addr);
    std::unique_ptr<Mapping> UnregisterMapping(const Mapping* mapping);

    bool Translate(uintptr_t virt, uintptr_t* phys);
    bool Populate(uintptr_t from, uintptr_t to);
    bool Fault(uintptr_t addr); 

private:
};

bool SetupMapping();

}  // namespace memory

#endif  // __MEMORY_SPACE_H__
