#ifndef __UTIL_ALLOCATOR_H__
#define __UTIL_ALLOCATOR_H__

#include "memory/memory.h"

namespace util {

template <typename T>
struct PhysicalAllocator {
    struct Header {
        memory::Contigous mem;
        union {
            T items[1];
        };

        Header() {}
    };

    static Header* FromPointer(T* ptr) {
        if (ptr == nullptr) {
            return nullptr;
        }

        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr) - Offset();
        Header* head = reinterpret_cast<Header*>(addr);
        if (head->mem.FromAddress() != addr) {
            return nullptr;
        }
        return head;
    }

    static size_t Offset() {
        Header head;
        return reinterpret_cast<uintptr_t>(&head.items[0]) -
            reinterpret_cast<uintptr_t>(&head.mem);
    }

    static size_t AllocationSize(size_t size) {
        if (size == 0) {
            return sizeof(Header);
        }
        return sizeof(Header) + (size - 1) * sizeof(T);
    }

    T* Allocate(size_t size);
    bool Grow(T* ptr, size_t size);
    bool Deallocate(T* ptr);
};

template <typename T>
T* PhysicalAllocator<T>::Allocate(size_t size) {
    memory::Contigous mem = memory::AllocatePhysical(AllocationSize(size)).release();
    if (mem.Size() == 0) {
        return nullptr;
    }

    Header *header = reinterpret_cast<Header*>(mem.FromAddress());
    ::new (static_cast<void*>(header)) Header();
    header->mem = mem;
    return header->items;
}

template <typename T>
bool PhysicalAllocator<T>::Grow(T* ptr, size_t size) {
    const Header* head = FromPointer(ptr);
    if (head == nullptr) {
        return false;
    }
    return head->mem.Size() >= AllocationSize(size);
}

template <typename T>
bool PhysicalAllocator<T>::Deallocate(T* ptr) {
    Header* head = FromPointer(ptr);
    if (head == nullptr) {
        return false;
    }

    memory::Contigous mem = Move(head->mem);
    head->~Header();
    memory::FreePhysical(Move(mem));
    return true;
}

}  // namespace util

#endif  // __UTIL_ALLOCATOR_H__
