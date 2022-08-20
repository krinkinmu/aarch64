#include "memory/cache.h"

#include <new>
#include <cstdint>
#include <utility>
#include <algorithm>

#include "common/math.h"

namespace memory {

namespace impl {

namespace {

size_t ObjectSize(size_t size, size_t alignment) {
    return common::AlignUp(std::max(size, sizeof(Storage)), alignment);
}

size_t SlabSize(size_t size, size_t control) {
    constexpr size_t kMinObjects = 8;
    constexpr size_t kMinSize = 4096;

    const size_t min_bytes = size * kMinObjects + control;

    if (kMinSize >= min_bytes) {
        return kMinSize;
    }

    const size_t order =  common::MostSignificantBit(min_bytes - 1) + 1;
    return static_cast<size_t>(1) << order;
}

Layout MakeLayout(size_t size, size_t alignment) {
    const size_t control_size = sizeof(Slab);
    const size_t object_size = ObjectSize(size, alignment);
    const size_t slab_size = SlabSize(object_size, control_size);

    Layout layout;
    layout.object_size = object_size;
    layout.object_offset = 0;
    layout.objects = (slab_size - control_size) / object_size;
    layout.control_offset = slab_size - control_size;
    layout.slab_size = slab_size;
    return layout;
}

[[ noreturn ]] void Panic() {
    while (1) {
        asm volatile("":::"memory");
    }
}

}  // namespace


Storage::Storage(void* ptr) : pointer(ptr) {}

Slab::Slab(const Cache* cache, Contigous mem, Layout layout)
        : cache_(cache), memory_(mem) {
    const uintptr_t from = memory_.FromAddress() + layout.object_offset;
    const uintptr_t to = from + layout.object_size * layout.objects;

    for (uintptr_t addr = from; addr < to; addr += layout.object_size) {
        void* ptr = reinterpret_cast<void*>(addr);
        Storage* storage = reinterpret_cast<Storage*>(ptr);
        ::new (storage) Storage(ptr);
        freelist_.PushBack(storage);
    }
}

Slab::~Slab() {
    if (Allocated() != 0) {
        Panic();
    }
    cache_ = nullptr;
}

const Cache* Slab::Owner() const { return cache_; }

Contigous Slab::Memory() const { return memory_; }

size_t Slab::Allocated() const { return allocated_; }

bool Slab::Empty() const { return freelist_.Empty(); }

void* Slab::Allocate() {
    Storage* storage = freelist_.PopFront();
    if (storage == nullptr) {
        return nullptr;
    }

    void* ptr = storage->pointer;
    storage->~Storage();
    ++allocated_;
    return ptr;
}

bool Slab::Free(void* ptr) {
    const uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    if (addr < memory_.FromAddress() || addr >= memory_.ToAddress()) {
        return false;
    }
    Storage* storage = reinterpret_cast<Storage*>(ptr);
    ::new (storage) Storage(ptr);
    freelist_.PushFront(storage);
    --allocated_;
    return true;
}


Allocator::Allocator(struct Layout layout) : allocated_(0), layout_(layout) {}

Slab* Allocator::Allocate(const Cache* cache) {
    Contigous memory = AllocatePhysical(layout_.slab_size).release();
    if (!memory.Size()) {
        return nullptr;
    }
    Slab* slab = reinterpret_cast<Slab*>(
            memory.FromAddress() + layout_.control_offset);
    ::new (slab) Slab(cache, memory, layout_);
    allocated_ += layout_.slab_size;
    return slab;
}

void Allocator::Free(Slab* slab) {
    Contigous memory = slab->Memory();
    slab->~Slab();
    allocated_ -= layout_.slab_size;
    FreePhysical(memory);
}

Slab* Allocator::Find(void* ptr) {
    const uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    const uintptr_t head = common::AlignDown(
        addr, static_cast<uintptr_t>(layout_.slab_size));
    Slab* slab = reinterpret_cast<Slab*>(head + layout_.control_offset);
    Contigous memory = slab->Memory();
    if (addr < memory.FromAddress() || addr >= memory.ToAddress()) {
        return nullptr;
    }
    return slab;
}

uintptr_t Allocator::Allocated() const { return allocated_; }

Layout Allocator::Layout() const { return layout_; }

}  // namespace impl


Cache::Cache(size_t size, size_t alignment)
    : layout_(impl::MakeLayout(size, alignment)), allocator_(layout_) {}

Cache::~Cache() {
    if (!partial_.Empty() || !full_.Empty()) {
        impl::Panic();
    }
    Reclaim();
}

size_t Cache::Allocated() const { return allocated_; }

size_t Cache::Occupied() const { return allocator_.Allocated(); }

size_t Cache::Reclaimable() const { return reclaimable_; }

bool Cache::Reclaim() {
    bool ret = Reclaimable() != 0;
    for (impl::Slab* slab = free_.PopFront();
         slab != nullptr;
         slab = free_.PopFront()) {
        allocator_.Free(slab);
    }
    reclaimable_ = 0;
    return ret;
}

void* Cache::Allocate() {
    if (!partial_.Empty()) {
        impl::Slab* slab = partial_.Front();
        if (slab->Allocated() + 1 == layout_.objects) {
            partial_.Unlink(slab);
            full_.PushFront(slab);
        }
        allocated_ += layout_.object_size;
        return slab->Allocate();
    }

    if (!free_.Empty()) {
        impl::Slab* slab = free_.PopFront();
        partial_.PushFront(slab);
        allocated_ += layout_.object_size;
        reclaimable_ -= layout_.slab_size;
        return slab->Allocate();
    }

    impl::Slab* slab = allocator_.Allocate(this);
    if (slab == nullptr) {
        return nullptr;
    }
    partial_.PushFront(slab);
    allocated_ += layout_.object_size;
    return slab->Allocate();
}

bool Cache::Free(void* ptr) {
    if (ptr == nullptr) {
        return false;
    }

    impl::Slab* slab = allocator_.Find(ptr);
    if (slab == nullptr) {
        return false;
    }

    if (slab->Owner() != this) {
        impl::Panic();
    }

    if (slab->Allocated() == 0) {
        return false;
    }

    if (!slab->Free(ptr)) {
        return false;
    }

    if (slab->Allocated() == 0) {
        partial_.Unlink(slab);
        free_.PushFront(slab);
        reclaimable_ += layout_.slab_size;
    }

    if (slab->Allocated() + 1 == layout_.objects) {
        full_.Unlink(slab);
        partial_.PushFront(slab);
    }

    allocated_ -= layout_.object_size;
    return true;
}

}  // namespace memory
