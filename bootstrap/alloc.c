#include "alloc.h"

#include <stdbool.h>
#include "list.h"


struct header {
    struct list_node link;
    size_t size;
    bool free;
};

struct footer {
    size_t size;
    bool free;
};


static const size_t ALIGNMENT = 8;
static struct list_head free;
static size_t allocated;


static uint64_t align_down(uint64_t addr, uintptr_t align)
{
    return addr & ~(align - 1);
}

static uint64_t align_up(uint64_t addr, uintptr_t align)
{
    return align_down(addr + align - 1, align);
}

static struct footer* matching_footer(struct header *header)
{
    uint64_t addr = align_down(
        (uint64_t)header + header->size - sizeof(struct footer),
        ALIGNMENT);

    return (void *)addr;
}

static struct footer* prev_footer(struct header *header)
{
    uint64_t addr = align_down(
        (uint64_t)header - sizeof(struct footer), ALIGNMENT);

    return (void *)addr;
}

static struct header* matching_header(struct footer *footer)
{
    uint64_t addr = align_up(
        (uint64_t)footer + sizeof(*footer), ALIGNMENT) - footer->size;

    return (void *)addr;
}

static struct header* next_header(struct footer *footer)
{
    uint64_t addr = align_up((uint64_t)footer + sizeof(*footer), ALIGNMENT);

    return (void *)addr;
}

void bootstrap_allocator_setup(void)
{
    free.head.next = &free.head;
    free.head.prev = &free.head;
}

intptr_t bootstrap_allocator_add_range(uint64_t begin, uint64_t end)
{
    const size_t metasz =
        align_up(sizeof(struct header), ALIGNMENT)
        + align_up(sizeof(struct footer), ALIGNMENT);
    const size_t minsz = 2 * metasz + ALIGNMENT;

    struct header *header = NULL;
    struct header *dummy_header = NULL;
    struct footer *footer = NULL;
    struct footer *dummy_footer = NULL;

    begin = align_up(begin, ALIGNMENT);
    end = align_down(end, ALIGNMENT);

    if (end < begin + minsz)
        return -1;

    dummy_footer = (void *)begin;
    dummy_footer->free = false;

    dummy_header = (void *)align_down(end - sizeof(struct header), ALIGNMENT);
    dummy_header->free = false;

    header = next_header(dummy_footer);
    footer = prev_footer(dummy_header);

    header->free = true;
    header->size = end - begin - metasz;
    footer->free = true;
    footer->size = end - begin - metasz;
    list_link_after(&free.head, &header->link);
    return 0;
}

intptr_t bootstrap_allocator_shutdown(void)
{
    if (allocated > 0)
        return -1;
    return 0;
}

static void *data_pointer(struct header *header)
{
    uint64_t addr = align_up((uint64_t)header + sizeof(*header), ALIGNMENT);

    return (void *)addr;
}

void *bootstrap_allocate(uintptr_t size)
{
    const size_t metasz =
        align_up(sizeof(struct header), ALIGNMENT)
        + align_up(sizeof(struct footer), ALIGNMENT);
    const size_t minsz = metasz + ALIGNMENT;

    struct list_node *head = &free.head;

    size = align_up(size, ALIGNMENT);

    for (struct list_node *ptr = head->next; ptr != head; ptr = ptr->next) {
        struct header *header = (void *)ptr;
        struct footer *footer = matching_footer(header);
        struct header *new_header = NULL;
        struct footer *new_footer = NULL;

        if (header->size < size + metasz)
            continue;

        // If after the allocation the remaining free memory in this range is
        // too small for another allocation, then we just return the whole
        // range.
        if (header->size < size + metasz + minsz) {
            list_unlink(&header->link);
            header->free = false;
            footer->free = false;
            allocated += header->size;
            return data_pointer(header);
        }

        // Otherwise we cut the current region in two pieces and return one of
        // them to the caller and the other back to the list of free ranges.
        // We do that by shrinking the current region and returning the tail of
        // the current region to the caller.
        header->size -= size + metasz;
        footer = matching_footer(header);
        footer->size = header->size;
        footer->free = true;

        new_header = next_header(footer);
        new_header->size = size + metasz;
        new_header->free = false;

        new_footer = matching_footer(new_header);
        new_footer->size = size + metasz;
        new_footer->free = false;

        allocated += new_header->size;
        return data_pointer(new_header);
    }

    return NULL;
}

static struct header* data_header(void *ptr)
{
    uint64_t addr = align_down(
        (uint64_t)ptr - sizeof(struct header), ALIGNMENT);

    return (void *)addr;
}

static void __bootstrap_free(void *ptr)
{
    struct header *header = data_header(ptr);
    struct footer *footer = matching_footer(header);

    struct footer *prev = prev_footer(header);
    struct header *next = next_header(footer);

    allocated -= header->size;

    // If the next range in memory is free, we remove it from the free list and
    // attach to the the range we are trying to free, so it looks like one big
    // busy memory range.
    if (next->free) {
        struct footer *next_footer = matching_footer(next);

        list_unlink(&next->link);
        header->size += next->size;
        footer = next_footer;
        footer->size = header->size;
    }

    // Similarly to the manipulations above, if the previous range in memory is
    // free, we attach it to the region we are freeing and working with them as
    // with one big region of memory.
    if (prev->free) {
        struct header *prev_header = matching_header(prev);

        list_unlink(&prev_header->link);
        prev_header->size += header->size;
        header = prev_header;
        footer->size = header->size;
    }

    header->free = true;
    footer->free = true;
    list_link_after(&free.head, &header->link);
}

void bootstrap_free(void *ptr)
{
    if (ptr == NULL)
        return;
    __bootstrap_free(ptr);
}

void *bootstrap_allocate_aligned(uintptr_t size, uintptr_t align)
{
    size_t data_size;
    uint64_t addr, data;

    if (align <= ALIGNMENT)
        return bootstrap_allocate(size);

    data_size = align_up(size, sizeof(uint64_t));
    addr = (uint64_t)bootstrap_allocate(data_size + sizeof(uint64_t) + align);
    if (addr == 0)
        return NULL;

    data = align_up(addr, align);
    *((uint64_t *)(data + data_size)) = addr;
    return (void *)data;
}

void bootstrap_free_aligned(void *ptr, uintptr_t size, uintptr_t align)
{
    size_t data_size;
    uint64_t addr, data;

    if (ptr == NULL)
        return;

    if (align <= ALIGNMENT) {
        __bootstrap_free(ptr);
        return;
    }

    data_size = align_up(size, sizeof(uint64_t));
    data = (uint64_t)ptr;
    addr = *((uint64_t *)(data + data_size));
    __bootstrap_free((void *)addr);
}
