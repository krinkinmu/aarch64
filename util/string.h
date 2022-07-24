#ifndef __UTIL_STRING_H__
#define __UTIL_STRING_H__

#include "util/stddef.h"

namespace util {

void *memcpy(void *dst, const void *src, size_t size);
void *memmove(void *dst, const void *src, size_t size);
void *memset(void *dst, int value, size_t size);

size_t strlen(const char *str);
int strcmp(const char *l, const char *r);
int strncmp(const char *l, const char *r, size_t size);

}  // namespace util

#endif  // __UTIL_STRING_H__
