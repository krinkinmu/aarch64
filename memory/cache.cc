#include "memory/cache.h"
#include "util/new.h"
#include "util/stdint.h"
#include "util/utility.h"
#include "util/math.h"
#include "util/algorithms.h"

namespace memory {

namespace {

size_t ObjectSize(size_t size, size_t alignment) {
    return util::AlignUp(util::Max(size, sizeof(cache::Storage)), alignment);
}

size_t SlabSize(size_t size, size_t control) {
    constexpr size_t kMinObjects = 8;
    constexpr size_t kMinSize = 4096;

    const size_t min_bytes = size * kMinObjects + control;

    if (kMinSize >= min_bytes) {
        return kMinSize;
    }

    const size_t order =  util::MostSignificantBit(min_bytes - 1) + 1;
    return static_cast<size_t>(1) << order;
}

cache::Layout MakeLayout(size_t size, size_t alignment) {
    const size_t control_size = sizeof(Slab);
    const size_t object_size = ObjectSize(size, alignment);
    const size_t slab_size = SlabSize(object_size, control_size);

    cache::Layout layout;
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

namespace cache {

Storage::Storage(void* ptr) : pointer(ptr) {}

}  // namespace cache

Slab::Slab(const Cache* cache, Contigous mem, cache::Layout layout)
        : cache_(cache), memory_(util::Move(mem)) {
    const uintptr_t from = memory_.FromAddress() + layout.object_offset;
    const uintptr_t to = from + layout.object_size * layout.objects;

    for (uintptr_t addr = from; addr < to; addr += layout.object_size) {
        cache::Storage* storage = reinterpret_cast<cache::Storage*>(addr);
        ::new (storage) cache::Storage(reinterpret_cast<void*>(addr));
        freelist_.LinkAt(freelist_.End(), storage);
    }
}

Slab::~Slab() {
    if (Allocated() != 0) {
        Panic();
    }
    cache_ = nullptr;
    FreePhysical(util::Move(memory_));
}

const Cache* Slab::Owner() const { return cache_; }

size_t Slab::Allocated() const { return allocated_; }

bool Slab::Empty() const { return freelist_.Empty(); }

void* Slab::Allocate() {
    if (Empty()) {
        return nullptr;
    }

    cache::Storage* storage = &*freelist_.Begin();
    freelist_.PopFront();
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
    cache::Storage* storage = reinterpret_cast<cache::Storage*>(ptr);
    ::new (storage) cache::Storage(ptr);
    freelist_.LinkAt(freelist_.Begin(), storage);
    --allocated_;
    return true;
}

Cache::Cache(size_t size, size_t alignment)
    : layout_(MakeLayout(size, alignment)) {}

Cache::~Cache() {
    Reclaim();
    while (!partial_.Empty() || !full_.Empty()) {}
}

size_t Cache::Allocated() const { return allocated_; }

size_t Cache::Occupied() const { return occupied_; }

size_t Cache::Reclaimable() const { return reclaimable_; }

bool Cache::Reclaim() {
    bool ret = Reclaimable() != 0;
    while (!free_.Empty()) {
        Slab* slab = &*free_.Begin();
        free_.Unlink(slab);
        FreeSlab(slab);
    }
    return ret;
}

void* Cache::Allocate() {
    if (!partial_.Empty()) {
        Slab* slab = &*partial_.Begin();
        if (slab->Allocated() + 1 == layout_.objects) {
            partial_.Unlink(slab);
            full_.LinkAt(full_.Begin(), slab);
        }
        allocated_ += layout_.object_size;
        return slab->Allocate();
    }

    if (!free_.Empty()) {
        Slab* slab = &*free_.Begin();
        free_.Unlink(slab);
        partial_.LinkAt(partial_.Begin(), slab);
        allocated_ += layout_.object_size;
        reclaimable_ -= layout_.slab_size;
        return slab->Allocate();
    }

    Slab* slab = AllocateSlab();
    if (slab == nullptr) {
        return nullptr;
    }
    partial_.LinkAt(partial_.Begin(), slab);
    allocated_ += layout_.object_size;
    return slab->Allocate();
}

bool Cache::Free(void* ptr) {
    Slab* slab = FindSlab(ptr);
    if (slab == nullptr) {
        return false;
    }

    if (slab->Allocated() == 0) {
        return false;
    }

    if (!slab->Free(ptr)) {
        return false;
    }

    if (slab->Allocated() == 0) {
        partial_.Unlink(slab);
        free_.LinkAt(free_.Begin(), slab);
        reclaimable_ += layout_.slab_size;
    }

    if (slab->Allocated() + 1 == layout_.objects) {
        full_.Unlink(slab);
        partial_.LinkAt(partial_.Begin(), slab);
    }

    allocated_ -= layout_.object_size;
    return true;
}

Slab* Cache::AllocateSlab() {
    Contigous memory = AllocatePhysical(layout_.slab_size).release();
    if (!memory.Size()) {
        return nullptr;
    }
    Slab* slab = reinterpret_cast<Slab*>(
            memory.FromAddress() + layout_.control_offset);
    ::new (slab) Slab(this, memory, layout_);
    occupied_ += layout_.slab_size;
    return slab;
}

void Cache::FreeSlab(Slab* slab) {
    slab->~Slab();
    occupied_ -= layout_.slab_size;
}

Slab* Cache::FindSlab(void* ptr) {
    const uintptr_t addr = util::AlignDown(
        reinterpret_cast<uintptr_t>(ptr),
        static_cast<uintptr_t>(layout_.slab_size));
    Slab* slab = reinterpret_cast<Slab*>(addr + layout_.control_offset);
    if (slab->Owner() != this) {
        return nullptr;
    }
    return slab;
}

}  // namespace memory
