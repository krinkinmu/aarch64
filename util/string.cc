#include "util/string.h"

namespace util {

int strcmp(const char *l, const char *r)
{
    while (*l == *r && *l != '\0') {
        ++l;
        ++r;
    }

    if (*l < *r)
        return -1;
    if (*l > *r)
        return 1;
    return 0;
}

int strncmp(const char *l, const char *r, size_t size)
{
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

void *memcpy(void *dst, const void *src, size_t size)
{
    unsigned char *to = static_cast<unsigned char *>(dst);
    const unsigned char *from = static_cast<const unsigned char *>(src);

    while (size-- > 0)
        *to++ = *from++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t size)
{
    unsigned char *to = static_cast<unsigned char *>(dst);
    const unsigned char *from = static_cast<const unsigned char *>(src);

    if (to <= from)
        while (size-- > 0)
            *to++ = *from++;
    else
        while (size-- > 0)
            to[size] = from[size];

    return dst;
}

void *memset(void *dst, int value, size_t size)
{
    unsigned char *to = static_cast<unsigned char *>(dst);

    while (size-- > 0)
        *to = static_cast<unsigned char>(value);
    return dst;
}

size_t strlen(const char *str)
{
    const char *begin = str;
    while (*str)
        ++str;
    return str - begin;
}

}  // namespace util
