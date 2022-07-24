#ifndef __MEMORY_SPACE_H__
#define __MEMORY_SPACE_H__

#include "util/stddef.h"
#include "util/stdint.h"


namespace memory {

enum class AccessMode {
    ReadOnly,
    ReadWrite,
    Executable,
};

struct PageTable;
struct Request;
struct Context;

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
