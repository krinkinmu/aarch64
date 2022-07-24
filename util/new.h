#ifndef __UTIL_NEW_H__
#define __UTIL_NEW_H__

#include "util/stddef.h"

void* operator new(size_t, void* ptr);

#endif  // __UTIL_NEW_H__
