#ifndef __MEMORY_CACHE_H__
#define __MEMORY_CACHE_H__

#include <cstddef>

#include "common/intrusive_list.h"
#include "memory.h"


namespace memory {

class Cache;

namespace impl {

struct Layout {
    size_t object_size;
    size_t object_offset;
    size_t objects;
    size_t control_offset;
    size_t slab_size;
};


struct Storage : public common::ListNode<Storage> {
    void* pointer;

    Storage(void* ptr);
};


class Slab : public common::ListNode<Slab> {
public:
    Slab(const Cache* cache, Contigous mem, Layout layout);
    ~Slab();

    Slab(const Slab& other) = delete;
    Slab& operator=(const Slab& other) = delete;
    Slab(Slab&& other) = delete;
    Slab& operator=(Slab&& other) = delete;

    const Cache* Owner() const;
    Contigous Memory() const;
    size_t Allocated() const;
    bool Empty() const;
    void* Allocate();
    bool Free(void* ptr);

private:
    common::IntrusiveList<Storage> freelist_;
    size_t allocated_ = 0;
    const Cache* cache_;
    Contigous memory_;
};

class Allocator {
public:
    Allocator(Layout layout);

    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;
    Allocator(Allocator&&) = default;
    Allocator& operator=(Allocator&&) = default;

    Slab* Allocate(const Cache* cache);
    void Free(Slab* slab);
    Slab* Find(void* ptr);

    uintptr_t Allocated() const;
    struct Layout Layout() const;

private:
    uintptr_t allocated_;
    struct Layout layout_;
};

}  // namespace impl


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
    size_t ObjectSize() const;

    bool Reclaim();
    void* Allocate();
    bool Free(void* ptr);

private:
    impl::Layout layout_;
    impl::Allocator allocator_;
    common::IntrusiveList<impl::Slab> free_;
    common::IntrusiveList<impl::Slab> partial_;
    common::IntrusiveList<impl::Slab> full_;
    uintptr_t allocated_ = 0;
    uintptr_t reclaimable_ = 0;
};

}  // namespace memory

#endif  // __MEMORY_SLAB_H__
