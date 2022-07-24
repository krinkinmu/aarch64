#include "fdt/scanner.h"

namespace fdt {

bool Range<uint32_t, uint32_t>::Parse(Scanner* pos) {
    return pos->ConsumeBe32(&begin) && pos->ConsumeBe32(&size);
}

bool Range<uint64_t, uint64_t>::Parse(Scanner* pos) {
    return pos->ConsumeBe64(&begin) && pos->ConsumeBe64(&size);
}

bool Range<uint32_t, uint64_t>::Parse(Scanner* pos) {
    return pos->ConsumeBe32(&begin) && pos->ConsumeBe64(&size);
}

bool Range<uint64_t, uint32_t>::Parse(Scanner* pos) {
    return pos->ConsumeBe64(&begin) && pos->ConsumeBe32(&size);
}

Scanner::Scanner() : data_(nullptr), size_(0), off_(0) {}

Scanner::Scanner(const uint8_t* data, size_t size)
    : data_(data), size_(size), off_(0) {}

bool Scanner::ConsumeBe32(uint32_t* val) {
    if (off_ + sizeof(uint32_t) > size_) {
        return false;
    }

    uint32_t x = 0;

    for (size_t i = 0; i < sizeof(uint32_t); i++) {
        x = (x << 8) | data_[off_ + i];
    }

    *val = x;
    off_ += sizeof(uint32_t);
    return true;
}

bool Scanner::ConsumeBe64(uint64_t* val) {
    if (off_ + sizeof(uint64_t) > size_) {
        return false;
    }

    uint64_t x = 0;

    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        x = (x << 8) | data_[off_ + i];
    }

    *val = x;
    off_ += sizeof(uint64_t);
    return true;
}

bool Scanner::ConsumeCstr(const char** str) {
    util::StringView s;

    if (!ConsumeCstr(&s)) {
        return false;
    }

    *str = s.Data();
    return true;
}

bool Scanner::ConsumeCstr(util::StringView* str) {
    const char *start = reinterpret_cast<const char*>(&data_[off_]);

    for (size_t size = 0; off_ + size < size_; size++) {
        if (start[size] == '\0') {
            *str = util::StringView(start, size);
            off_ += size + 1;
            return true;
        }
    }

    return false;
}

bool Scanner::ConsumeBytes(size_t size, const uint8_t** data) {
    if (off_ + size > size_) {
        return false;
    }

    *data = &data_[off_];
    off_ += size;
    return true;
}

bool Scanner::ConsumeToken(Token* token) {
    Scanner copy = *this;
    uint32_t val = 0;

    if (!copy.ConsumeBe32(&val)) {
        return false;
    }

    switch (val) {
    case static_cast<uint32_t>(Token::BEGIN_NODE):
    case static_cast<uint32_t>(Token::END_NODE):
    case static_cast<uint32_t>(Token::PROP):
    case static_cast<uint32_t>(Token::NOP):
    case static_cast<uint32_t>(Token::END):
        *token = static_cast<Token>(val);
        break;
    default:
        return false;
    }

    *this = copy;
    return true;
}

bool Scanner::AlignForward(size_t alignment) {
    if (alignment == 0 || off_ % alignment == 0) {
        return true;
    }

    const size_t shift = alignment - off_ % alignment;

    if (off_ + shift > size_) {
        return false;
    }

    off_ += shift;
    return true;
}

bool operator==(const Scanner& l, const Scanner& r) {
    uintptr_t laddr = reinterpret_cast<uintptr_t>(l.data_) + l.off_;
    uintptr_t raddr = reinterpret_cast<uintptr_t>(r.data_) + r.off_;
    return laddr == raddr;
}

bool operator!=(const Scanner& l, const Scanner& r) {
    return !(l == r);
}

}  // namespace fdt
