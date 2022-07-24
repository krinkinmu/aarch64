#ifndef __UTIL_STDDEF_H__
#define __UTIL_STDDEF_H__

#include <stddef.h>

namespace util {

using ::ptrdiff_t;
using ::size_t;
typedef decltype(nullptr) nullptr_t;

}  // namespace util

#endif  // __UTIL_STDDEF_H__
