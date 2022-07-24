#ifndef __UTIL_ALGORITHMS_H__
#define __UTIL_ALGORITHMS_H__

#include "util/utility.h"
#include "util/stddef.h"

namespace util {

template <typename T>
const T& Max(const T& l, const T& r) {
    return (l < r) ? r : l;
}

template <typename T>
const T& Min(const T& l, const T& r) {
    return (l < r) ? l : r;
}

template <typename T>
void Swap(T& l, T& r) {
    T copy = Move(l);
    l = Move(r);
    r = Move(copy);
}

template <typename It>
void Reverse(It begin, It end) {
    while (begin < end) {
        --end;
        Swap(*begin, *end);
        ++begin;
    }
}

template <typename It>
void RotateLeft(It begin, It end, size_t shift) {
    Reverse(begin, begin + shift);
    Reverse(begin + shift, end);
    Reverse(begin, end);
}

template <typename It>
void RotateRight(It begin, It end, size_t shift) {
    RotateLeft(begin, end, end - begin - shift);
}

template <typename It, typename T, typename Less>
It LowerBound(It begin, It end, const T& key, Less less) {
    for (It it = begin; it != end; ++it) {
        if (!less(*it, key)) {
            return it;
        }
    }
    return end;
}

template <typename It, typename T, typename Less>
It UpperBound(It begin, It end, const T& key, Less less) {
    for (It it = begin; it != end; ++it) {
        if (less(key, *it)) {
            return it;
        }
    }
    return end;
}

}  // namespace util

#endif  // __UTIL_ALGORITHMS_H__
