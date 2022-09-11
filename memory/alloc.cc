#include "alloc.h"

#include <cstring>

#include "cache.h"
#include "memory.h"
#include "common/math.h"

namespace memory {

namespace {

constexpr size_t kMaxAlignment = 32;

struct Metadata {
    Cache* cache;
    Contigous mem;   
};

Cache caches[] = {
    Cache(128, 128),
    Cache(256, 256),
    Cache(384, 384),
    Cache(512, 512),
    Cache(640, 640),
    Cache(768, 768),
    Cache(896, 896),
    Cache(1024, 1024),
    Cache(1152, 1152),
    Cache(1280, 1280),
    Cache(1408, 1408),
    Cache(1536, 1536),
    Cache(1664, 1664),
    Cache(1792, 1792),
    Cache(1920, 1920),
    Cache(2048, 2048),
    Cache(2176, 2176),
    Cache(2304, 2304),
    Cache(2432, 2432),
    Cache(2560, 2560),
    Cache(2688, 2688),
    Cache(2816, 2816),
    Cache(2944, 2944),
    Cache(3072, 3072),
    Cache(3200, 3200),
    Cache(3328, 3328),
    Cache(3456, 3456),
    Cache(3584, 3584),
    Cache(3712, 3712),
    Cache(3840, 3840),
    Cache(3968, 3968),
    Cache(4096, 4096),
};

Cache* CacheFor(size_t size) {
    const size_t index = size / 128;
    if (index >= sizeof(caches)/sizeof(caches[0])) {
        return nullptr;
    }
    return &caches[index];
}

constexpr size_t MetadataSize() {
    return common::AlignUp(sizeof(Metadata), kMaxAlignment);
}

Metadata* MetadataFor(const void* ptr) {
    return reinterpret_cast<Metadata*>(
        reinterpret_cast<uintptr_t>(ptr) - MetadataSize());
}

}  // namespace

void* Allocate(size_t size) {
    const size_t allocation_size = size + MetadataSize();

    Cache* cache = CacheFor(allocation_size);
    if (cache != nullptr) {
        void* ptr = cache->Allocate();
        if (ptr == nullptr) {
            return nullptr;
        }

        Metadata* m = reinterpret_cast<Metadata*>(ptr);
        m->cache = cache;
        m->mem = Contigous(nullptr);
        return reinterpret_cast<void*>(
            reinterpret_cast<uintptr_t>(ptr) + MetadataSize());
    }

    auto mem = AllocatePhysical(allocation_size);
    if (mem) {
        Metadata* m = reinterpret_cast<Metadata*>(mem->FromAddress());
        m->cache = nullptr;
        m->mem = *mem;
        return reinterpret_cast<void*>(mem->FromAddress() + MetadataSize());
    }

    return nullptr;
}

void* Reallocate(void* ptr, size_t new_size) {
    Metadata* m = MetadataFor(ptr);
    size_t old_size = 0;

    if (m->cache != nullptr) {
        Cache* cache = m->cache;
        if (cache->ObjectSize() >= new_size) {
            return ptr;
        }
        old_size = cache->ObjectSize() - MetadataSize();
    }

    if (m->mem != Contigous(nullptr)) {
        if (m->mem.Size() >= new_size) {
            return ptr;
        }
        old_size = m->mem.Size() - MetadataSize();
    }

    void* new_ptr = Allocate(new_size);
    if (new_ptr != nullptr) {
        memcpy(new_ptr, ptr, old_size);
        Free(ptr);
        return new_ptr;
    }
    return nullptr;
}

void Free(void* ptr) {
    Metadata* m = MetadataFor(ptr);

    if (m->cache != nullptr) {
        Cache* cache = m->cache;
        cache->Free(reinterpret_cast<void*>(m));
        return;
    }

    FreePhysical(m->mem);
}

}  // namespace memory
