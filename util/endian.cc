#include "util/endian.h"

namespace util {

namespace {

uint16_t bswap16(uint16_t x)
{
    return (x >> 8) | ((x & 0xff) << 16);
}

uint32_t bswap32(uint32_t x)
{
    return bswap16(x >> 16) | ((uint32_t)bswap16(x & 0xffff) << 16);
}

uint64_t bswap64(uint64_t x)
{
    return bswap32(x >> 32) | ((uint64_t)bswap32(x & 0xffffffff) << 32);
}

}  // namepsace


uint8_t FromBigEndian(be8_t v) { return v; }
be8_t ToBigEndian(uint8_t v) { return v; }

uint8_t FromLittleEndian(le8_t v) { return v; }
le8_t ToLittleEndian(uint8_t v) { return v; }

#ifdef __LITTLE_ENDIAN__

uint64_t FromBigEndian(be64_t v) { return bswap64(v); }
uint32_t FromBigEndian(be32_t v) { return bswap32(v); }
uint16_t FromBigEndian(be16_t v) { return bswap16(v); }

be64_t ToBigEndian(uint64_t v) { return bswap64(v); }
be32_t ToBigEndian(uint32_t v) { return bswap32(v); }
be16_t ToBigEndian(uint16_t v) { return bswap16(v); }

uint64_t FromLittleEndian(le64_t v) { return v; }
uint32_t FromLittleEndian(le32_t v) { return v; }
uint16_t FromLittleEndian(le16_t v) { return v; }

le64_t ToLittleEndian(uint64_t v) { return v; }
le32_t ToLittleEndian(uint32_t v) { return v; }
le16_t ToLittleEndian(uint16_t v) { return v; }

#elif __BIG_ENDIAN__

uint64_t FromBigEndian(be64_t v) { return v; }
uint32_t FromBigEndian(be32_t v) { return v; }
uint16_t FromBigEndian(be16_t v) { return v; }

be64_t ToBigEndian(uint64_t v) { return v; }
be32_t ToBigEndian(uint32_t v) { return v; }
be16_t ToBigEndian(uint16_t v) { return v; }

uint64_t FromLittleEndian(le64_t v) { return bswap64(v); }
uint32_t FromLittleEndian(le32_t v) { return bswap32(v); }
uint16_t FromLittleEndian(le16_t v) { return bswap16(v); }

le64_t ToLittleEndian(uint64_t v) { return bswap64(v); }
le32_t ToLittleEndian(uint32_t v) { return bswap32(v); }
le16_t ToLittleEndian(uint16_t v) { return bswap16(v); }

#else

class enum endianness {
    BIG,
    LITTLE,
};

static enum endianness detect(void)
{
    uint32_t x = 1;

    if (*reinterpret_cast<uint8_t *>(&x) == 1)
        return LITTLE;
    return BIG;
}

uint64_t FromBigEndian(be64_t v)
{
    if (detect() == LITTLE)
        return bswap64(v);
    return v;
}

uint32_t FromBigEndian(be32_t v)
{
    if (detect() == LITTLE)
        return bswap32(v);
    return v;
}

uint16_t FromBigEndian(be16_t v)
{
    if (detect() == LITTLE)
        return bswap16(v);
    return v;
}

be64_t ToBigEndian(uint64_t v)
{
    if (detect() == LITTLE)
        return bswap64(v);
    return v;
}

be32_t ToBigEndian(uint32_t v)
{
    if (detect() == LITTLE)
        return bswap32(v);
    return v;
}

be16_t ToBigEndian(uint16_t v)
{
    if (detect() == LITTLE)
        return bswap16(v);
    return v;
}

uint64_t FromLittleEndian(le64_t v)
{
    if (detect() == LITTLE)
        return v;
    return bswap64(v);
}

uint32_t FromLittleEndian(le32_t v)
{
    if (detect() == LITTLE)
        return v;
    return bswap32(v);
}

uint16_t FromLittleEndian(le16_t v)
{
    if (detect() == LITTLE)
        return v;
    return bswap16(v);
}

le64_t ToLittleEndian(uint64_t v)
{
    if (detect() == LITTLE)
        return v;
    return bswap64(v);
}

le32_t ToLittleEndian(uint32_t v)
{
    if (detect() == LITTLE)
        return v;
    return bswap32(v);
}

le16_t ToLittleEndian(uint16_t v)
{
    if (detect() == LITTLE)
        return v;
    return bswap16(v);
}

#endif

}  // namespace util
