#ifndef __MEMORY_SPACE_H__
#define __MEMORY_SPACE_H__

#include <cstddef>
#include <cstdint>


namespace memory {

namespace impl {

class PageTable {
public:
    static constexpr size_t kPageTableSize = 512;

    PageTable(Contigous mem);

    PageTable(const PageTable& other) = delete;
    PageTable& operator=(const PageTable& other) = delete;
    PageTable(PageTable&& other) = delete;
    PageTable& operator=(PageTable&& other) = delete;

    uintptr_t Address() const;
    

private:
    Contigous memory_;
    uint64_t *descriptors_;
};

}  // namespace impl


class AddressSpace {
public:
    AddressSpace();
    ~AddressSpace();


    AddressSpace(AddressSpace&& other) = default;
    AddressSpace& operator=(AddressSpace&& other) = default;

    AddressSpace(AddressSpace& other) = delete;
    AddressSpace& operator=(AddressSpace& other) = delete;


    bool Map(uintptr_t phys, uintptr_t virt, size_t size, AccessMode mode);
    bool Remap(uintptr_t virt, size_t size, AccessMode mode);

    bool IoMap(uintptr_t phys, uintptr_t virt, size_t size, AccessMode mode);
    bool IoRemap(uintptr_t virt, size_t size, AccessMode mode);


    uintptr_t Base() const;

private:

    bool MapInternal(
        uintptr_t virt, uintptr_t phys, size_t size, uint64_t attr);


    bool MapTableEntry(const Request& request, const Context& context);
    bool MapTableEntries(
        const Request& request, PageTable* table, size_t level);

    PageTable* AllocatePageTable();
    void FreePageTable(PageTable* table);
    void Clear(PageTable* table);

    PageTable* root_;
};


class MemoryMap;

bool SetupAddressSpace(const MemoryMap& mmap, AddressSpace* space);
bool SetupMapping(const AddressSpace& space);

}  // namespace memory

#endif  // __MEMORY_SPACE_H__
