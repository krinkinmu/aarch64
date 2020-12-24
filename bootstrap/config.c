#include "config.h"

static uint64_t devicetree_begin, devicetree_end;
static struct reserved_memory_range ranges[128];
static uint32_t reserved;

static uint8_t heap[0x200000] __attribute__((aligned(4096)));

void reserved_ranges(struct reserved_memory_range **rngs, uint32_t *size)
{
    *rngs = ranges;
    *size = reserved;
}

void devicetree(uint64_t *begin, uint64_t *end)
{
    *begin = devicetree_begin;
    *end = devicetree_end;
}

void bootstrap_heap(uint64_t *begin, uint64_t *end)
{
    *begin = (uint64_t)heap;
    *end = (uint64_t)heap + sizeof(heap);
}

int reserve_range(uint64_t begin, uint64_t end, uint64_t type)
{
    if (reserved == sizeof(ranges) / sizeof(ranges[0]))
        return -1;

    if (type == RESERVE_DEVICETREE) {
        devicetree_begin = begin;
        devicetree_end = end;
    }

    ranges[reserved].type = type;
    ranges[reserved].begin = begin;
    ranges[reserved].end = end;
    ++reserved;
    return 0;
}
