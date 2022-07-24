#include "bootstrap/memory.h"
#include "fdt/span.h"

namespace {

template <typename It>
bool RegisterRegions(It begin, It end, memory::MemoryMap* mmap) {
    for (It it = begin; it != end; ++it) {
        const uintptr_t from = it->begin;
        const uintptr_t to = from + it->size;

        if (!mmap->Register(from, to, memory::MemoryStatus::FREE)) {
            return false;
        }
    }
    return true;
}

bool RegisterRegions(
        const fdt::Property& property,
        size_t address, size_t size,
        memory::MemoryMap* mmap) {
    if (address == 1 && size == 1) {
        fdt::Span<fdt::Range<uint32_t, uint32_t>> span;
        if (!property.ValueAsSpan(&span)) {
            return false;
        }
        return RegisterRegions(span.ConstBegin(), span.ConstEnd(), mmap);
    }

    if (address == 2 && size == 2) {
        fdt::Span<fdt::Range<uint64_t, uint64_t>> span;
        if (!property.ValueAsSpan(&span)) {
            return false;
        }
        return RegisterRegions(span.ConstBegin(), span.ConstEnd(), mmap);
    }

    if (address == 1 && size == 2) {
        fdt::Span<fdt::Range<uint32_t, uint64_t>> span;
        if (!property.ValueAsSpan(&span)) {
            return false;
        }
        return RegisterRegions(span.ConstBegin(), span.ConstEnd(), mmap);
    }

    if (address == 2 && size == 1) {
        fdt::Span<fdt::Range<uint64_t, uint32_t>> span;
        if (!property.ValueAsSpan(&span)) {
            return false;
        }
        return RegisterRegions(span.ConstBegin(), span.ConstEnd(), mmap);
    }

    return false;
}

bool ParseMemoryNode(
        const fdt::Blob& blob, fdt::Scanner pos,
        size_t address, size_t size, memory::MemoryMap* mmap) {
    fdt::Node node;
    fdt::Property property;
    fdt::Token token;

    while (blob.TokenAt(pos, &token)) {
        switch (token) {
        case fdt::Token::PROP:
            if (!blob.ConsumeProperty(&pos, &property)) {
                return false;
            }
            if (property.name == "reg") {
                return RegisterRegions(property, address, size, mmap);
            }
            break;
        case fdt::Token::END_NODE:
            return true;
        default:
            return false;
        }
    }
    return false;
}

bool RegisterMemory(const fdt::Blob& blob, memory::MemoryMap* mmap) {
    fdt::Scanner pos = blob.Root().offset;
    uint32_t address_cells = 2;
    uint32_t size_cells = 2;
    fdt::Node node;
    fdt::Property property;
    fdt::Token token;

    while (blob.TokenAt(pos, &token)) {
        switch (token) {
        case fdt::Token::END:
            return true;
        case fdt::Token::PROP:
            if (!blob.ConsumeProperty(&pos, &property)) {
                return false;
            }
            if (property.name == "#size-cells") {
                if (!property.ValueAsBe32(&size_cells)) {
                    return false;
                }
            }
            if (property.name == "#address-cells") {
                if (!property.ValueAsBe32(&address_cells)) {
                    return false;
                }
            }
            break;
        case fdt::Token::BEGIN_NODE:
            if (!blob.ConsumeStartNode(&pos, &node)) {
                return false;
            }
            if (node.name.StartsWith("memory")) {
                if (!ParseMemoryNode(
                        blob, pos, address_cells, size_cells, mmap)) {
                    return false;
                }
            }
            if (!blob.SkipNode(&pos)) {
                return false;
            }
            break;
        case fdt::Token::NOP:
            if (!blob.ConsumeNop(&pos)) {
                return false;
            }
            break;
        case fdt::Token::END_NODE:
            if (!blob.ConsumeEndNode(&pos)) {
                return false;
            }
            break;
        }
    }
    return false;
}

}  // namespace

bool MMapFromDTB(const fdt::Blob& blob, memory::MemoryMap* mmap) {
    if (!RegisterMemory(blob, mmap)) {
        return false;
    }

    const auto reserved = blob.Reserved();
    for (auto it = reserved.ConstBegin(); it != reserved.ConstEnd(); ++it) {
        const uintptr_t begin = it->begin;
        const uintptr_t end = begin + it->size;

        if (!mmap->Reserve(begin, end)) {
            return false;
        }
    }

    return true;
}
