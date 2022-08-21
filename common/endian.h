#ifndef __COMMON_ENDIAN_H__
#define __COMMON_ENDIAN_H__

#include <cstdint>

namespace common {

typedef uint64_t be64_t;
typedef uint32_t be32_t;
typedef uint16_t be16_t;
typedef uint8_t be8_t;

typedef uint64_t le64_t;
typedef uint32_t le32_t;
typedef uint16_t le16_t;
typedef uint8_t le8_t;

uint64_t FromBigEndian(be64_t v);
uint32_t FromBigEndian(be32_t v);
uint16_t FromBigEndian(be16_t v);
uint8_t FromBigEndian(be8_t v);

be64_t ToBigEndian(uint64_t v);
be32_t ToBigEndian(uint32_t v);
be16_t ToBigEndian(uint16_t v);
be8_t ToBigEndian(uint8_t v);

uint64_t FromLittleEndian(le64_t v);
uint32_t FromLittleEndian(le32_t v);
uint16_t FromLittleEndian(le16_t v);
uint8_t FromLittleEndian(le8_t v);

le64_t ToLittleEndian(uint64_t v);
le32_t ToLittleEndian(uint32_t v);
le16_t ToLittleEndian(uint16_t v);
le8_t ToLittleEndian(uint8_t v);

}  // namespace common

#endif  // __COMMON_ENDIAN_H__
