#include "string.h"

size_t strlen(const char* str) {
    const char* b = str;
    while (*str) str++;
    return str - b;
}
