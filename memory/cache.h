#ifndef __MEMORY_CACHE_H__
#define __MEMORY_CACHE_H__

#include "memory/memory.h"
#include "util/intrusive_list.h"
#include "util/stddef.h"


namespace memory {

namespace cache {

struct Layout {
    size_t object_size;
    size_t object_offset;
    size_t objects;
    size_t control_offset;
    size_t slab_size;
};

struct Storage : public util::ListNode<Storage> {
    void* pointer;

    Storage(void* ptr);
};

}  // namespace cache

class Cache;

class Slab : public util::ListNode<Slab> {
public:
    Slab(const Cache* cache, Contigous mem, cache::Layout layout);
    ~Slab();

    Slab(const Slab& other) = delete;
    Slab& operator=(const Slab& other) = delete;
    Slab(Slab&& other) = delete;
    Slab& operator=(Slab&& other) = delete;

    const Cache* Owner() const;
    size_t Allocated() const;
    bool Empty() const;
    void* Allocate();
    bool Free(void* ptr);

private:
    util::IntrusiveList<cache::Storage> freelist_;
    size_t allocated_ = 0;
    const Cache* cache_;
    Contigous memory_;
};

class Cache {
public:
    Cache(size_t size, size_t alignment);
    ~Cache();

    Cache(const Cache& other) = delete;
    Cache& operator=(const Cache& other) = delete;
    Cache(Cache&& other) = delete;
    Cache& operator=(Cache&& other) = delete;

    size_t Allocated() const;
    size_t Occupied() const;
    size_t Reclaimable() const;

    bool Reclaim();
    void* Allocate();
    bool Free(void* ptr);

private:
    Slab* AllocateSlab();
    Slab* FindSlab(void* ptr);
    void FreeSlab(Slab* slab);

    util::IntrusiveList<Slab> free_;
    util::IntrusiveList<Slab> partial_;
    util::IntrusiveList<Slab> full_;
    uintptr_t allocated_ = 0;
    uintptr_t occupied_ = 0;
    uintptr_t reclaimable_ = 0;
    cache::Layout layout_;
};

}  // namespace memory

#endif  // __MEMORY_SLAB_H__
