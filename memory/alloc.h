#ifndef __MEMORY_ALLOC_H__
#define __MEMORY_ALLOC_H__

#include <cstddef>

namespace memory {

void* Allocate(size_t size);
void* Reallocate(void* ptr, size_t new_size);
void Free(void* ptr);

}  // namespace memory

#endif  // __MEMORY_ALLOC_H__
