#ifndef __C_STRING_H__
#define __C_STRING_H__

#include <stddef.h>

size_t strlen(const char* str);
void* memset(void* dst, int c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);

int strncmp(const char *l, const char *r, size_t size);

#endif  // __C_STRING_H__
