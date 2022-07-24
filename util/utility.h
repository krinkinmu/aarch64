#ifndef __UTIL_UTILITY_H__
#define __UTIL_UTILITY_H__

#include "util/type_traits.h"

namespace util {

template <typename T>
typename RemoveReference<T>::type&&
Forward(typename RemoveReference<T>::type&& arg) {
    static_assert(!IsReference<T>::value);
    return static_cast<typename RemoveReference<T>::type&&>(arg);
}

template <typename T>
typename RemoveReference<T>::type&&
Forward(typename RemoveReference<T>::type& arg) {
    return static_cast<typename RemoveReference<T>::type&&>(arg);
}

template <typename T>
typename RemoveReference<T>::type&& Move(T&& arg) {
    return static_cast<typename RemoveReference<T>::type&&>(arg);
}

}  // namespace util

#endif  // __UTIL_UTILITY_H__
