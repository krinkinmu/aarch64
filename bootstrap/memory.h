#ifndef __BOOTSTRAP_MEMORY_H__
#define __BOOTSTRAP_MEMORY_H__

#include "fdt/blob.h"
#include "memory/phys.h"

bool MMapFromDTB(const fdt::Blob& blob, memory::MemoryMap* mmap);

#endif  // __BOOTSTRAP_MEMORY_H__
