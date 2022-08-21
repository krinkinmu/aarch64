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

void* memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = dst;
    const unsigned char* s = src;

    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];
    return dst;
}

int strncmp(const char *l, const char *r, size_t size) {
    if (size == 0)
        return 0;

    while (size > 1 && *l == *r && *l != '\0') {
        ++l;
        ++r;
        --size;
    }

    if (*l < *r)
        return -1;
    if (*l > *r)
        return 1;
    return 0;
}
