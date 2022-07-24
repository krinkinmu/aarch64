#include <stddef.h>

/*
 * Compilers sometimes refer to standard C library functions even when they are
 * not used directly in the code. For example, a compiler may generate a call
 * to memset function instead of directly generating code that fills a
 * structure or an array with 0s.
 *
 * Given that we are not using the default C library, such references will
 * result in a linker error. To avoid that we provide our own implementations
 * of the C library functions that compiler expects.
 */
void* memset(void* dst, int value, size_t num) {
    char *p = dst;
    for (size_t i = 0; i != num; ++i) {
        p[i] = value;
    }
    return dst;
}
