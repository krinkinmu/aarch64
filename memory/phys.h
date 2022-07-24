#ifndef __MEMORY_PHYS_H__
#define __MEMORY_PHYS_H__

#include "util/fixed_vector.h"
#include "util/stddef.h"
#include "util/stdint.h"

namespace memory {

enum class MemoryStatus {
    RESERVED,
    FREE,
};

struct MemoryRange {
    uintptr_t begin;
    uintptr_t end;
    MemoryStatus status;
};

class MemoryMap {
public:
    MemoryMap() {}

    MemoryMap(const MemoryMap& other) = default;
    MemoryMap(MemoryMap&& other) = default;
    MemoryMap& operator=(const MemoryMap& other) = default;
    MemoryMap& operator=(MemoryMap&& other) = default;

    bool Register(uintptr_t begin, uintptr_t end, MemoryStatus status);
    bool Reserve(uintptr_t begin, uintptr_t end);
    bool Release(uintptr_t begin, uintptr_t end);
    bool AllocateIn(
        uintptr_t begin, uintptr_t end,
        size_t size, size_t alignment,
        uintptr_t *ret);
    bool Allocate(size_t size, size_t alignment, uintptr_t* ret);

    const MemoryRange* ConstBegin() const { return ranges_.ConstBegin(); }
    const MemoryRange* ConstEnd() const { return ranges_.ConstEnd(); }

private:
    bool SetStatus(uintptr_t begin, uintptr_t end, MemoryStatus status);
    bool FindIn(
        uintptr_t begin, uintptr_t end,
        size_t size, size_t alignment, MemoryStatus status,
        uintptr_t* ret);

    util::FixedVector<MemoryRange, 128> ranges_;
};

}  // namespace memory

#endif  // __MEMORY_PHYS_H__
