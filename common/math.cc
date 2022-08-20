#include "common/math.h"


namespace common {

int LeastSignificantBit(uint64_t x) {
    if (x == 0) {
        return 64;
    }

    int lsb = 0;

    if ((x & 0xffffffff) == 0) {
        lsb += 32;
        x >>= 32;
    }
    if ((x & 0xffff) == 0) {
        lsb += 16;
        x >>= 16;
    }
    if ((x & 0xff) == 0) {
        lsb += 8;
        x >>= 8;
    }
    if ((x & 0xf) == 0) {
        lsb += 4;
        x >>= 4;
    }
    if ((x & 0x3) == 0) {
        lsb += 2;
        x >>= 2;
    }
    if ((x & 0x1) == 0) {
        lsb += 1;
        x >>= 1;
    }
    return lsb;
}

int MostSignificantBit(uint64_t x) {
    if (x == 0) {
        return -1;
    }

    int msb = 0;

    if ((x & 0xffffffff00000000) != 0) {
        msb += 32;
        x >>= 32;
    }
    if ((x & 0xffff0000) != 0) {
        msb += 16;
        x >>= 16;
    }
    if ((x & 0xff00) != 0) {
        msb += 8;
        x >>= 8;
    }
    if ((x & 0xf0) != 0) {
        msb += 4;
        x >>= 4;
    }
    if ((x & 0xc) != 0) {
        msb += 2;
        x >>= 2;
    }
    if ((x & 0x2) != 0) {
        msb += 1;
        x >>= 1;
    }
    return msb;
}

}  // namespace common

