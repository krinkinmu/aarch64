#ifndef __FDT_BLOB_H__
#define __FDT_BLOB_H__

#include "fdt/scanner.h"
#include "fdt/span.h"
#include "util/string_view.h"


namespace fdt {

struct Property {
    util::StringView name;
    const uint8_t *data = nullptr;
    size_t size = 0;

    Property() = default;
    Property(const util::StringView& name, const uint8_t *data, size_t size)
    : name(name), data(data), size(size) {}

    bool ValueAsBe32(uint32_t* data) const;
    bool ValueAsBe64(uint64_t* data) const;

    template <typename T>
    bool ValueAsSpan(Span<T>* span) const {
        if (size % detail::Parser<T>::kWireSize != 0) {
            return false;
        }
        *span = Span<T>(data, size / detail::Parser<T>::kWireSize);
        return true;
    }
};


struct Node {
    util::StringView name;
    Scanner offset;

    Node() = default;
    Node(const util::StringView& name, Scanner off) : name(name), offset(off) {}
};


class Blob {
public:
    struct Header {
        uint32_t magic = 0;
        uint32_t totalsize = 0;
        uint32_t off_dt_struct = 0;
        uint32_t off_dt_strings = 0;
        uint32_t off_mem_rsvmap = 0;
        uint32_t version = 0;
        uint32_t last_comp_version = 0;
        uint32_t boot_cpuid_phys = 0;
        uint32_t size_dt_strings = 0;
        uint32_t size_dt_struct = 0;
    };

    static bool Parse(const uint8_t *data, size_t size, Blob *blob);

    Blob() : str_(nullptr), strsz_(0) {}

    Blob(const Blob& other) = default;
    Blob(Blob&& other) = default;
    Blob& operator=(const Blob& other) = default;
    Blob& operator=(Blob&& other) = default;

    uint32_t Version() const { return header_.version; }
    uint32_t BootCPU() const { return header_.boot_cpuid_phys; }

    Node Root() const { return root_; }
    Span<Range<uint64_t, uint64_t>> Reserved() const { return reserved_; }

    bool SkipNode(Scanner* pos) const;
    bool TokenAt(const Scanner& pos, Token* token) const;
    bool ConsumeStartNode(Scanner* pos, Node* node) const;
    bool ConsumeEndNode(Scanner* pos) const;
    bool ConsumeProperty(Scanner* pos, Property* property) const;
    bool ConsumeNop(Scanner* pos) const;

private:
    Blob(
        Header header,
        Node root,
        Span<Range<uint64_t, uint64_t>> reserved,
        const uint8_t *str, size_t strsz)
    : header_(header)
    , root_(root)
    , reserved_(reserved)
    , str_(str)
    , strsz_(strsz)
    {}

    Header header_;
    Node root_;
    Span<Range<uint64_t, uint64_t>> reserved_;
    const uint8_t *str_;
    size_t strsz_;
};

}  // namespace fdt

#endif  // __FDT_BLOB_H__
