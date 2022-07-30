#include "string.h"

size_t strlen(const char* str) {
    const char* b = str;
    while (*str) str++;
    return str - b;
}

void* memset(void* dst, int c, size_t n) {
    char* s = (char *)dst;
    for (size_t i = 0; i < n; ++i) s[i] = c;
    return dst;
}
