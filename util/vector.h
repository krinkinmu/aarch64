#ifndef __UTIL_VECTOR_H__
#define __UTIL_VECTOR_H__

#include "util/algorithms.h"
#include "bootstrap/log.h"

namespace util {

template <typename T, typename A>
class Vector : private A {
public:
    Vector() {}
    Vector(A a) : A(a) {}

    ~Vector() {
        Clear();
        A::Deallocate(items_);
    }

    Vector(const Vector& other) = delete;
    Vector& operator=(const Vector& other) = delete;

    Vector(Vector&& other);
    Vector& operator=(Vector&& other);

    bool Assign(size_t count, const T& item);
    void Swap(Vector& other);

    bool PushBack(const T& item);
    template <typename... Args>
    bool EmplateBack(Args&&... args);

    bool Insert(const T* pos, const T& item);

    template <typename... Args>
    bool Emplace(const T* pos, Args&&... args);

    T* Erase(T* pos);
    const T* Erase(const T* pos);
    T* Erase(T* begin, T* end);
    const T* Erase(const T* begin, const T* end);

    bool PopBack();
    bool Resize(size_t size) { return Resize(size, T()); }
    bool Resize(size_t size, const T& value);

    void Clear() {
        for (size_t i = 0; i < size_; ++i) {
            items_[i].~T();
        }
        size_ = 0;
    }

    T& Front() { return items_[0]; }
    const T& Front() const { return items_[0]; }
    T& Back() { return items_[size_ - 1]; }
    const T& Back() const { return items_[size_ - 1]; }

    T& At(size_t pos) { return items_[pos]; }
    const T& At(size_t pos) const { return items_[pos]; }
    T& operator[](size_t pos) { return At(pos); }
    const T& operator[](size_t pos) const { return At(pos); }

    T* Data() { return items_; }
    const T* Data() const { return items_; }

    bool Empty() const { return size_ == 0; }
    size_t Size() const { return size_; }
    size_t Capacity() const { return capacity_; }

    T* Begin() { return items_; }
    const T* ConstBegin() const { return items_; }
    T* End() { return items_ + size_; }
    const T* ConstEnd() const { return items_ + size_; }

private:
    bool Reallocate(size_t capacity) {
        T* items = A::Allocate(capacity);
        if (items == nullptr) {
            return false;
        }

        for (size_t i = 0; i < size_; ++i) {
            ::new(static_cast<void*>(&items[i])) T(Move(items_[i]));
            items_[i].~T();
        }

        A::Deallocate(items_);
        items_ = items;
        capacity_ = capacity;
        return true;
    }

    bool Grow(size_t capacity) {
        if (Capacity() >= capacity) {
            return true;
        }

        capacity = Max(Capacity() * 2 / 3, capacity);
        if (A::Grow(items_, capacity)) {
            capacity_ = capacity;
            return true;
        }

        return Reallocate(capacity);
    }

    void PlaceAt(T* pos, const T& t) {
        ::new(static_cast<void*>(pos)) T(t);
    }

    void PlaceAt(T& pos, T&& t) {
        ::new(static_cast<void*>(pos)) T(Move(t));
    }

    template <typename... Args>
    void PlaceAt(T* pos, Args&&... args) {
        ::new(static_cast<void*>(pos)) T(Forward<Args>(args)...);
    }

    T *items_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename T, typename A>
Vector<T, A>::Vector(Vector&& other) {
    Vector empty;
    empty.Swap(*this);
    Swap(other);
}

template <typename T, typename A>
Vector<T, A>& Vector<T, A>::operator=(Vector&& other) {
    if (this != &other) {
        Vector empty;
        empty.Swap(*this);
        Swap(other);
    }
    return *this;
}

template <typename T, typename A>
bool Vector<T, A>::Assign(size_t count, const T& item) {
    if (!Grow(count)) {
        return false;
    }

    Clear();
    for (size_t i = 0; i != count; ++i) {
        PlaceAt(&items_[i], item);
    }
    size_ = count;
    return true;
}

template <typename T, typename A>
void Vector<T, A>::Swap(Vector& other) {
    Swap(items_, other.items_);
    Swap(size_, other.size_);
    Swap(capacity_, other.capacity_);
}

template <typename T, typename A>
bool Vector<T, A>::PushBack(const T& item) {
    if (!Grow(size_ + 1)) {
        return false;
    }
    PlaceAt(&items_[size_++], item);
    return true;
}

template <typename T, typename A>
template <typename... Args>
bool Vector<T, A>::EmplateBack(Args&&... args) {
    if (!Grow(size_ + 1)) {
        return false;
    }

    PlaceAt(&items_[size_++], Forward<Args>(args)...);
    return true;
}

template <typename T, typename A>
bool Vector<T, A>::Insert(const T* pos, const T& item) {
    size_t off = pos - ConstBegin();
    if (!Grow(size_ + 1)) {
        return false;
    }

    PlaceAt(&items_[size_++], item);
    RotateRight(Begin() + off, items_ + size_, 1);
    return true;
}

template <typename T, typename A>
template <typename... Args>
bool Vector<T, A>::Emplace(const T* pos, Args&&... args) {
    size_t off = pos - ConstBegin();
    if (!Grow(size_ + 1)) {
        return false;
    }

    PlaceAt(&items_[size_++], Forward(args)...);
    RotateRight(Begin() + off, items_ + size_, 1);
    return true;
}

template <typename T, typename A>
T* Vector<T, A>::Erase(T* pos) {
    RotateLeft(pos, End(), 1);
    items_[--size_].~T();
    return pos;
}

template <typename T, typename A>
const T* Vector<T, A>::Erase(const T* pos) {
    return Erase(const_cast<T*>(pos));
}

template <typename T, typename A>
T* Vector<T, A>::Erase(T* begin, T* end) {
    const size_t count = end - begin;
    RotateLeft(begin, End(), count);
    for (T* it = End() - count; it != End(); ++it) {
        it->~T();
    }
    size_ -= count;
    return begin;
}

template <typename T, typename A>
const T* Vector<T, A>::Erase(const T* begin, const T* end) {
    return Erase(const_cast<T*>(begin), const_cast<T*>(end));
}

template <typename T, typename A>
bool Vector<T, A>::PopBack() {
    if (Empty()) {
        return false;
    }

    items_[--size_].~T();
    return true;
}

template <typename T, typename A>
bool Vector<T, A>::Resize(size_t size, const T& item) {
    if (!Grow(size)) {
        return false;
    }

    if (size > Size()) {
        for (size_t i = Size(); i < size; ++i) {
            PlaceAt(&items_[i], item);
        }
    } else {
        for (size_t i = size; i < Size(); ++i) {
            items_[i].~T();
        }
    }
    size_ = size;
    return true;
}

}  // util

#endif  // __UTIL_VECTOR_H__
