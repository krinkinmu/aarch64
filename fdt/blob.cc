#include "fdt/blob.h"

namespace fdt {

namespace {

const uint32_t kAlignment = 4;
const uint32_t kVersion = 17;
const uint32_t kMagic = 0xd00dfeed;

bool ParseReserved(
        Scanner scanner, Span<Range<uint64_t, uint64_t>>* reserved) {
    Range<uint64_t, uint64_t> range;
    for (size_t count = 0; scanner.ConsumeRange(&range); ++count) {
        if (range.begin == 0 && range.size == 0) {
            *reserved = Span<Range<uint64_t, uint64_t>>(
                    scanner.Data(), count);
            return true;
        }
    }
    return false;
}

bool ParseHeader(Scanner* scanner, Blob::Header *header) {
    if (!scanner->ConsumeBe32(&header->magic)) {
        return false;
    }
    if (!scanner->ConsumeBe32(&header->totalsize)) {
        return false;
    }
    if (!scanner->ConsumeBe32(&header->off_dt_struct)) {
        return false;
    }
    if (!scanner->ConsumeBe32(&header->off_dt_strings)) {
        return false;
    }
    if (!scanner->ConsumeBe32(&header->off_mem_rsvmap)) {
        return false;
    }
    if (!scanner->ConsumeBe32(&header->version)) {
        return false;
    }
    if (!scanner->ConsumeBe32(&header->last_comp_version)) {
        return false;
    }
    if (!scanner->ConsumeBe32(&header->boot_cpuid_phys)) {
        return false;
    }
    if (!scanner->ConsumeBe32(&header->size_dt_strings)) {
        return false;
    }
    if (!scanner->ConsumeBe32(&header->size_dt_struct)) {
        return false;
    }
    if (header->magic != kMagic) {
        return false;
    }
    if (header->totalsize > scanner->Size()) {
        return false;
    }
    if (header->last_comp_version > kVersion) {
        return false;
    }
    if (header->off_mem_rsvmap + 2 * sizeof(uint64_t) > header->totalsize) {
        return false;
    }
    if (header->off_dt_strings + header->size_dt_strings > header->totalsize) {
        return false;
    }
    if (header->off_dt_struct + header->size_dt_struct > header->totalsize) {
        return false;
    }
    return true;
}

bool EnsureToken(Scanner* pos, Token token) {
    Token actual;

    if (!pos->ConsumeToken(&actual)) {
        return false;
    }

    return actual == token;
}

bool ParseStartNode(Scanner *pos, Node *node) {
    if (!EnsureToken(pos, Token::BEGIN_NODE)) {
        return false;
    }

    util::StringView name;

    if (!pos->ConsumeCstr(&name)) {
        return false;
    }

    if (!pos->AlignForward(kAlignment)) {
        return false;
    }

    *node = Node(name, *pos);
    return true;
}

}  // namespace

bool Property::ValueAsBe32(uint32_t* value) const {
    Scanner scan(data, size);

    if (!scan.ConsumeBe32(value)) {
        return false;
    }
    return true;
}

bool Property::ValueAsBe64(uint64_t* value) const {
    Scanner scan(data, size);

    if (!scan.ConsumeBe64(value)) {
        return false;
    }
    return true;
}

bool Blob::SkipNode(Scanner* pos) const {
    Scanner copy = *pos;
    Node node;
    Property property;
    int depth = 1;

    while (true) {
        Token token;
        if (!TokenAt(copy, &token)) {
            return false;
        }
        switch (token) {
        case Token::BEGIN_NODE:
            if (!ConsumeStartNode(&copy, &node)) {
                return false;
            }
            ++depth;
            break;
        case Token::END_NODE:
            if (!ConsumeEndNode(&copy)) {
                return false;
            }
            if (--depth == 0) {
                *pos = copy;
                return true;
            }
            break;
        case Token::PROP:
            if (!ConsumeProperty(&copy, &property)) {
                return false;
            }
            break;
        case Token::NOP:
            if (!ConsumeNop(&copy)) {
                return false;
            }
            break;
        case Token::END:
            return false;
        }
    }
    return false;
}

bool Blob::TokenAt(const Scanner& pos, Token* token) const {
    Scanner copy = pos;
    return copy.ConsumeToken(token);
}

bool Blob::ConsumeStartNode(Scanner* pos, Node* node) const {
    Scanner copy = *pos;

    if (!ParseStartNode(&copy, node)) {
        return false;
    }
    *pos = copy;
    return true;
}

bool Blob::ConsumeProperty(Scanner* pos, Property *property) const {
    Scanner copy = *pos;

    if (!EnsureToken(&copy, Token::PROP)) {
        return false;
    }

    uint32_t size = 0;

    if (!copy.ConsumeBe32(&size)) {
        return false;
    }

    uint32_t off = 0;

    if (!copy.ConsumeBe32(&off)) {
        return false;
    }

    if (off >= strsz_) {
        return false;
    }

    const uint8_t *data = nullptr;

    if (!copy.ConsumeBytes(size, &data)) {
        return false;
    }

    if (!copy.AlignForward(kAlignment)) {
        return false;
    }

    util::StringView name;
    Scanner str(&str_[off], strsz_ - off);

    if (!str.ConsumeCstr(&name)) {
        return false;
    }

    *property = Property(name, data, size);
    *pos = copy;
    return true;
}

bool Blob::ConsumeEndNode(Scanner* pos) const {
    return EnsureToken(pos, Token::END_NODE); 
}

bool Blob::ConsumeNop(Scanner* pos) const {
    return EnsureToken(pos, Token::NOP);
}

bool Blob::Parse(const uint8_t *data, size_t size, Blob *blob) {
    Scanner scanner(data, size);
    Header header;

    if (!ParseHeader(&scanner, &header)) {
        return false;
    }

    Scanner nodes(data + header.off_dt_struct, header.size_dt_struct);
    Node root;

    if (!ParseStartNode(&nodes, &root)) {
        return false;
    }

    Scanner rsv(
            data + header.off_mem_rsvmap,
            header.totalsize - header.off_mem_rsvmap);
    Span<Range<uint64_t, uint64_t>> reserved;
    if (!ParseReserved(rsv, &reserved)) {
        return false;
    }

    const uint8_t *str = data + header.off_dt_strings;
    const size_t strsz = header.size_dt_strings;

    *blob = Blob(header, root, reserved, str, strsz);
    return true;
}

}  // namespace fdt
