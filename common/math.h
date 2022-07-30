#ifndef __COMMON_MATH_H__
#define __COMMON_MATH_H__

#include <cstdint>

namespace common {

int LeastSignificantBit(uint64_t x);
int MostSignificantBit(uint64_t x);

template <typename T>
T AlignDown(T x, T alignment) {
    return x & ~(alignment - 1);
}

template <typename T>
T AlignUp(T x, T alignment) {
    return AlignDown(x + alignment - 1, alignment);
}

template <typename T>
T Clamp(T x, T from, T to) {
    if (x < from) {
        x = from;
    }
    if (x > to) {
        x = to;
    }
    return x;
}

template <typename T>
T Bits(T x, unsigned from, unsigned to) {
    const T m1 = ~((static_cast<T>(1) << from) - 1);
    const T m2 = ~((static_cast<T>(1) << to) - 1);
    return x & m1 & m2;
}

}  // namespace common

#endif  // __COMMON_MATH_H__
