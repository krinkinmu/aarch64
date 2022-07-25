#include "common/stream.h"

#include <stddef.h>
#include <string.h>


namespace common {

namespace {

char* FormatNumber(char* buffer, unsigned long long x) {
    if (x == 0) {
        *buffer++ = '0';
        return buffer;
    }

    char* e = buffer;
    while (x) {
        *e++ = (x % 10) + '0';
        x /= 10;
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

OutputStream& operator<<(OutputStream& out, long long x) {
    char buffer[32];
    char* e = buffer;
    if (x < 0) {
        *e++ = '-';
        x = -x;
    }
    e = FormatNumber(e, static_cast<unsigned long long>(x));
    Write(out, buffer, e - buffer);
    return out;
}

OutputStream& operator<<(OutputStream& out, unsigned long long x) {
    char buffer[32];
    char* e = FormatNumber(buffer, x);
    Write(out, buffer, e - buffer);
    return out;
}

OutputStream& operator<<(OutputStream& out, const char* str) {
    Write(out, str, strlen(str));
    return out;
}

}  // namespace common

