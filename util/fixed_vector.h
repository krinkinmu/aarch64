#ifndef __UTIL_FIXED_VECTOR_H__
#define __UTIL_FIXED_VECTOR_H__

#include "util/algorithms.h"
#include "util/stddef.h"
#include "util/new.h"

namespace util {

template <typename T, size_t N>
class FixedVector {
public:
    FixedVector() {}
    ~FixedVector() { Clear(); }

    FixedVector(const FixedVector& other);
    FixedVector(FixedVector&& other);
    FixedVector& operator=(const FixedVector& other);
    FixedVector& operator=(FixedVector&& other);

    bool Assign(size_t count, const T& item);

    bool PushBack(const T& item);
    template <typename... Args>
    bool EmplaceBack(Args&&... args);

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
    size_t MaxSize() const { return N; }
    size_t Capacity() const { return N; }

    T* Begin() { return items_; }
    const T* ConstBegin() const { return items_; }

    T* End() { return items_ + size_; }
    const T* ConstEnd() const { return items_ + size_; }

private:
    bool Copy(const T* begin, const T* end) {
        const size_t count = end - begin;
        if (count > Capacity()) {
            return false;
        }
        Clear();
        for (const T* it = begin; it != end; ++it) {
            PlaceAt(&items_[it - begin], *it);
        }
        size_ = count;
        return true;
    }

    bool Move(T* begin, T* end) {
        const size_t count = end - begin;
        if (count > Capacity()) {
            return false;
        }
        Clear();
        for (T* it = begin; it != end; ++it) {
            PlaceAt(&items_[it - begin], Move(*it));
        }
        size_ = count;
        return true;
    }

    void PlaceAt(T* pos, const T& t) {
        ::new(static_cast<void*>(pos)) T(t);
    }

    void PlaceAt(T* pos, T&& t) {
        ::new(static_cast<void*>(pos)) T(Move(t));
    }

    template <typename... Args>
    void PlaceAt(T* pos, Args&&... args) {
        ::new(static_cast<void*>(pos)) T(Forward<Args>(args)...);
    }

    union {
        T items_[N];
    };
    size_t size_ = 0;
};


template <typename T, size_t N>
FixedVector<T, N>::FixedVector(const FixedVector& other) {
    Copy(other.ConstBegin(), other.ConstEnd());
}

template <typename T, size_t N>
FixedVector<T, N>::FixedVector(FixedVector&& other) {
    Move(other.Begin(), other.End());    
}

template <typename T, size_t N>
FixedVector<T, N>& FixedVector<T, N>::operator=(const FixedVector& other) {
    if (this != &other) {
        Assign(other.ConstBegin(), other.ConstEnd());
    }
    return *this;
}

template <typename T, size_t N>
FixedVector<T, N>& FixedVector<T, N>::operator=(FixedVector&& other) {
    if (this != &other) {
        Move(other.Begin(), other.End());
    }
    return *this;
}

template <typename T, size_t N>
bool FixedVector<T, N>::Assign(size_t count, const T& item) {
    if (count > Capacity()) {
        return false;
    }
    Clear();
    for (size_t i = 0; i != count; ++i) {
        PlaceAt(&items_[i], item);
    }
    size_ = count;
    return true;
}

template <typename T, size_t N>
bool FixedVector<T, N>::PushBack(const T& item) {
    if (size_ == Capacity()) {
        return false;
    }
    PlaceAt(&items_[size_++], item);
    return true;
}

template <typename T, size_t N>
template <typename... Args>
bool FixedVector<T, N>::EmplaceBack(Args&&... args) {
    if (size_ == Capacity()) {
        return false;
    }
    PlaceAt(&items_[size_++], Forward<Args>(args)...);
    return true;
}

template <typename T, size_t N>
bool FixedVector<T, N>::Insert(const T* pos, const T& item) {
    if (size_ == Capacity()) {
        return false;
    }

    PlaceAt(&items_[size_++], item);
    RotateRight(const_cast<T*>(pos), items_ + size_, 1);
    return true;
}

template <typename T, size_t N>
template <typename... Args>
bool FixedVector<T, N>::Emplace(const T* pos, Args&&... args) {
    if (size_ == Capacity()) {
        return false;
    }
    PlaceAt(&items_[size_++], Forward<Args>(args)...);
    RotateRight(pos, items_ + size_, 1);
    return true;
}

template <typename T, size_t N>
T* FixedVector<T, N>::Erase(T* pos) {
    RotateLeft(pos, End(), 1);
    items_[--size_].~T();
    return pos;
}

template <typename T, size_t N>
const T* FixedVector<T, N>::Erase(const T* pos) {
    return Erase(const_cast<T*>(pos));
}

template <typename T, size_t N>
T* FixedVector<T, N>::Erase(T* begin, T* end) {
    const size_t count = end - begin;
    RotateLeft(begin, End(), count);
    for (T* it = End() - count; it != End(); ++it) {
        it->~T();
    }
    size_ -= count;
    return begin;
}

template <typename T, size_t N>
const T* FixedVector<T, N>::Erase(const T* begin, const T* end) {
    return Erase(const_cast<T*>(begin), const_cast<T*>(end));
}

template <typename T, size_t N>
bool FixedVector<T, N>::PopBack() {
    if (Empty()) {
        return false;
    }

    items_[--size_].~T();
    return true;
}

template <typename T, size_t N>
bool FixedVector<T, N>::Resize(size_t size, const T& value) {
    if (size > Capacity()) {
        return false;
    }

    if (size > Size()) {
        for (size_t i = Size(); i < size; ++i) {
            PlaceAt(&items_[i], value);
        }
    } else {
        for (size_t i = size; i < Size(); ++i) {
            items_[i].~T();
        }
    }
    size_ = size;
    return true;
}

}  // namespace util

#endif  // __UTIL_FIXED_VECTOR_H__
