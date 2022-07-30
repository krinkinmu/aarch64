#include "common/stream.h"

#include <cstdint>
#include <cstddef>
#include <cstring>


namespace common {

namespace {

char* FormatNumber(char* buffer, int base, unsigned long long x) {
    static const char digit[] = "0123456789abcdef";
    if (x == 0) {
        *buffer++ = '0';
        return buffer;
    }

    char* e = buffer;
    while (x) {
        *e++ = digit[x % base];
        x /= base;
    }

    for (char *l = buffer, *r = e; l < r;) {
        const char c = *l;
        --r;
        *l = *r;
        *r = c;
        l++;
    }

    return e;
}

void Write(OutputStream& out, const char* data, size_t n) {
    for (size_t written = 0; written < n;) {
        const int ret = out.PutN(&data[written], n - written);
        if (ret < 0) {
            return;
        }
        written += ret;
    }
}

}  // namespace

OutputStream::~OutputStream() {}

int OutputStream::PutN(const char* data, int n) {
    for (int i = 0; i < n; ++i) {
        const int ret = this->Put(data[i]);
        if (ret < 0) {
            if (i > 0) {
                return i;
            }
            return ret;
        }
    }
    return n;
}

OutputStream& operator<<(OutputStream& out, char c) {
    out.Put(c);
    return out;
}

OutputStream& operator<<(OutputStream& out, int x) {
    return out << static_cast<long long>(x);
}

OutputStream& operator<<(OutputStream& out, unsigned x) {
    return out << static_cast<unsigned long long>(x);
}

OutputStream& operator<<(OutputStream& out, long x) {
    return out << static_cast<long long>(x);
}

OutputStream& operator<<(OutputStream& out, unsigned long x) {
    return out << static_cast<unsigned long long>(x);
}

OutputStream& operator<<(OutputStream& out, long long x) {
    if (x < 0) {
        return out << "-" << static_cast<unsigned long long>(-x);
    }
    return out << static_cast<unsigned long long>(x);
}

OutputStream& operator<<(OutputStream& out, unsigned long long x) {
    char buffer[128];
    char* e = FormatNumber(buffer, 10, x);
    Write(out, buffer, e - buffer);
    return out;
}

OutputStream& operator<<(OutputStream& out, const char* str) {
    Write(out, str, strlen(str));
    return out;
}

OutputStream& operator<<(OutputStream& out, const void* ptr) {
    const uintptr_t x = reinterpret_cast<uintptr_t>(ptr);
    char buffer[128] = "0x";
    char* e = FormatNumber(buffer + 2, 16, x);
    Write(out, buffer, e - buffer);
    return out;
}

}  // namespace common

