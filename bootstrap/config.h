#ifndef __BOOTSTRAP_CONFIG_H__
#define __BOOTSTRAP_CONFIG_H__

#include <stdint.h>


static const uint64_t RESERVE_KERNEL = 0x01;
static const uint64_t RESERVE_DEVICETREE = 0x02;
static const uint64_t RESERVE_OTHER = 0x03;

struct reserved_memory_range {
    uint64_t type;
    uint64_t begin;
    uint64_t end;
};

void reserved_ranges(struct reserved_memory_range **ranges, uint32_t *size);
void devicetree(uint64_t *begin, uint64_t *end);
void bootstrap_heap(uint64_t *begin, uint64_t *end);

int reserve_range(uint64_t begin, uint64_t end, uint64_t type);

#endif  // __BOOTSTRAP_CONFIG_H__
