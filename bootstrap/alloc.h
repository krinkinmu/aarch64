#ifndef __BOOTSTRAP_ALLOC_H__
#define __BOOTSTRAP_ALLOC_H__

#include <stddef.h>
#include <stdint.h>


// This is an early generic allocator implementation. It will only exist
// during early initialization of the system and we eventually will have
// to shut it down once the normal memory subsystem is initialized.
//
// Due to the nature of this early allocation system it's not performance
// critical, doesn't need to support concurrency and should have small
// memory footprint. More importantly however we should be able to
// cleanly shut it down. So it's important to keep track of the allocated
// memory to be able to verify during the shutdown that there is no
// outstanding allocated memory.


// Initialize all the internal metadata of the early memory allocator.
void bootstrap_allocator_setup(void);

// Add the given range of addresses to the free memory pool of the early
// memory allocator. Once added this memory range might be allocated upon
// allocation request. On success returns zero. The function may fail if
// the region is not suitable for memory allocation, for example, if it's
// too small.
intptr_t bootstrap_allocator_add_range(uint64_t begin, uint64_t end);

// Shut down the early allocator. On success the function returns zero.
// The function fails if there is still some outstanding allocated memory
// that wasn't freed.
intptr_t bootstrap_allocator_shutdown(void);

// Allocate a region of memory of the given size. In case of success a
// non-zero pointer is returned, otherwise a zero-pointer is returned.
void *bootstrap_allocate(uintptr_t size);

// Frees a region of memory previously allocated using early_allocate
// function. It's ok to call this function with a zero pointer. This
// function cannot fail if it's given the correct pointer.
void boostrap_free(void *ptr);

// Similar to the early_allocate but supports additional alignment
// requirements that the early_allocate doesn't. Normally,
// early_allocate should be enough as the data it returns is aligned to
// the 8-byte boundary, however if for whatever reason you want a larger
// alignment you can use this function.
void *bootstrap_allocate_aligned(uintptr_t size, uintptr_t align);

// For aligned allocations we need to keep track of some additional
// bookkeeping information, so we have to use a matching free function
// that is aware of that additional information.
void bootstrap_free_aligned(void *ptr, uintptr_t size, uintptr_t align);

#endif  // __BOOTSTRAP_ALLOC_H__
