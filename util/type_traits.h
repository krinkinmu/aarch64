#ifndef __UTIL_TYPE_TRAITS_H__
#define __UTIL_TYPE_TRAITS_H__

namespace util {

template <typename T> struct IsReference
{ static constexpr bool value = false; };
template <typename T> struct IsReference<T&>
{ static constexpr bool value = true; };

template <typename T> struct RemoveReference { using type = T; };
template <typename T> struct RemoveReference<T&> { using type = T; };
template <typename T> struct RemoveReference<T&&> { using type = T; };

}  // namespace util

#endif  // __UTIL_TYPE_TRAITS_H__
