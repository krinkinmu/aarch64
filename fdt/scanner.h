#ifndef __FDT_SCANNER_H__
#define __FDT_SCANNER_H__

#include "util/stddef.h"
#include "util/stdint.h"
#include "util/string_view.h"

namespace fdt {

enum class Token {
    BEGIN_NODE = 1,
    END_NODE = 2,
    PROP = 3,
    NOP = 4,
    END = 9,
};


class Scanner;

template <typename B, typename S>
struct Range;

template <>
struct Range<uint32_t, uint32_t> {
    uint32_t begin;
    uint32_t size;

    bool Parse(Scanner* pos);
};

template <>
struct Range<uint64_t, uint64_t> {
    uint64_t begin;
    uint64_t size;

    bool Parse(Scanner* pos);
};

template <>
struct Range<uint32_t, uint64_t> {
    uint32_t begin;
    uint64_t size;

    bool Parse(Scanner* pos);
};

template <>
struct Range<uint64_t, uint32_t> {
    uint64_t begin;
    uint32_t size;

    bool Parse(Scanner* pos);
};


class Scanner {
public:
    Scanner();
    Scanner(const uint8_t* data, size_t size);

    Scanner(const Scanner& other) = default;
    Scanner(Scanner&& other) = default;
    Scanner& operator=(const Scanner& other) = default;
    Scanner& operator=(Scanner&& other) = default;

    const uint8_t* Data() const { return data_; }
    size_t Size() const { return size_; }
    size_t Offset() const { return off_; }

    bool ConsumeBe32(uint32_t* val);
    bool ConsumeBe64(uint64_t* val);
    bool ConsumeCstr(const char** str);
    bool ConsumeCstr(util::StringView* str);
    bool ConsumeBytes(size_t size, const uint8_t** data);
    bool ConsumeToken(Token* token);
    bool AlignForward(size_t alignment);

    template <typename B, typename S>
    bool ConsumeRange(Range<B, S>* range) {
        Scanner copy = *this;
        if (!range->Parse(&copy)) {
            return false;
        }
        *this = copy;
        return true;
    }

    friend bool operator==(const Scanner& l, const Scanner& r);

private:
    const uint8_t* data_;
    size_t size_;
    size_t off_;
};

bool operator!=(const Scanner& l, const Scanner& r);

}  // namespace fdt

#endif  // __FDT_SCANNER_H__
