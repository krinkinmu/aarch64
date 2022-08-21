#ifndef __FDT_SPAN_H__
#define __FDT_SPAN_H__

#include <cstddef>

#include "fdt/scanner.h"


namespace fdt {

namespace detail {

template <typename T>
struct Parser;

template <>
struct Parser<uint32_t> {
    static constexpr size_t kWireSize = 4;

    static bool Parse(Scanner* pos, uint32_t* item) {
        return pos->ConsumeBe32(item);
    }
};

template <>
struct Parser<uint64_t> {
    static constexpr size_t kWireSize = 8;

    static bool Parse(Scanner* pos, uint64_t* item) {
        return pos->ConsumeBe64(item);
    }
};

template <>
struct Parser<Range<uint32_t, uint32_t>> {
    static constexpr size_t kWireSize = 8;

    static bool Parse(Scanner* pos, Range<uint32_t, uint32_t>* item) {
        return pos->ConsumeRange(item);
    }
};

template <>
struct Parser<Range<uint64_t, uint64_t>> {
    static constexpr size_t kWireSize = 16;

    static bool Parse(Scanner* pos, Range<uint64_t, uint64_t>* item) {
        return pos->ConsumeRange(item);
    }
};

template <>
struct Parser<Range<uint32_t, uint64_t>> {
    static constexpr size_t kWireSize = 12;

    static bool Parse(Scanner* pos, Range<uint32_t, uint64_t>* item) {
        return pos->ConsumeRange(item);
    }
};

template <>
struct Parser<Range<uint64_t, uint32_t>> {
    static constexpr size_t kWireSize = 12;

    static bool Parse(Scanner* pos, Range<uint64_t, uint32_t>* item) {
        return pos->ConsumeRange(item);
    }
};

}  // namespace detail


template <typename T> class Span;

template <typename T>
class SpanIterator {
public:
    SpanIterator() {}
    SpanIterator(const Span<T>* span, size_t pos) : span_(span), pos_(pos) {}

    SpanIterator(const SpanIterator& other) = default;
    SpanIterator& operator=(const SpanIterator& other) = default;

    SpanIterator& operator++() {
        ++pos_;
        return *this;
    }

    SpanIterator operator++(int) {
        SpanIterator copy = *this;
        ++pos_;
        return copy;
    }

    SpanIterator& operator--() {
        --pos_;
        return *this;
    }

    SpanIterator operator--(int) {
        SpanIterator copy = *this;
        --pos_;
        return *this;
    }

    const T* operator->() const;

    T operator*() const;

    template <typename U>
    friend bool operator==(const SpanIterator<U>& l, const SpanIterator<U>& r);

private:
    const Span<T>* span_ = nullptr;
    size_t pos_ = 0;
    mutable T buf_;
};


template <typename T>
class Span {
public:
    Span(const uint8_t* data, size_t size)
            : data_(data), size_(detail::Parser<T>::kWireSize * size) {}
    Span() {}

    Span(const Span& other) = default;
    Span& operator=(const Span& other) = default;

    size_t Size() const { return size_ / detail::Parser<T>::kWireSize; }
    bool Empty() const { return size_ == 0; }
    T At(size_t pos) const;
    T operator[](size_t pos) const { return At(pos); }

    SpanIterator<T> ConstBegin() const { return SpanIterator<T>(this, 0); }
    SpanIterator<T> ConstEnd() const { return SpanIterator<T>(this, Size()); }

private:
    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
};

template <typename T>
T Span<T>::At(size_t pos) const {
    const size_t offset = pos * detail::Parser<T>::kWireSize;

    Scanner scanner(data_ + offset, size_ - offset);
    T item;
    detail::Parser<T>::Parse(&scanner, &item);
    return item;
}

template <typename T>
const T* SpanIterator<T>::operator->() const {
    buf_ = span_->At(pos_);
    return &buf_;
}

template <typename T>
T SpanIterator<T>::operator*() const {
    return span_->At(pos_);
}

template <typename T>
bool operator==(const SpanIterator<T>& l, const SpanIterator<T>& r) {
    return l.span_ == r.span_ && l.pos_ == r.pos_;
}

template <typename T>
bool operator!=(const SpanIterator<T>& l, const SpanIterator<T>& r) {
    return !(l == r);
}

}  // namespace fdt

#endif  // __FDT_SPAN_H__
