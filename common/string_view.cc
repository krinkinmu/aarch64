#include "string_view.h"

#include <algorithm>
#include <cstring>


namespace common {

StringView::StringView() : data_(nullptr), size_(0) {}

StringView::StringView(const char* str) : data_(str), size_(strlen(str)) {}

StringView::StringView(const char* data, size_t size)
        : data_(data), size_(size) {}

size_t StringView::Size() const { return size_; }

size_t StringView::Length() const { return size_; }

bool StringView::Empty() const { return size_ == 0; }

char StringView::operator[](size_t pos) const { return data_[pos]; }

const char* StringView::Data() const { return data_; }

int StringView::Compare(const StringView& other) const {
    const size_t len = std::min(Size(), other.Size());
    const int rc = strncmp(data_, other.data_, len);

    if (rc != 0) {
        return rc;
    }

    if (Size() != other.Size()) {
        return Size() < other.Size() ? -1 : 1;
    }

    return 0;
}

StringView StringView::Substr(size_t from, size_t count) const {
    size_t to = from + count;
    if (from > size_) {
        from = size_;
    }
    if (to > size_) {
        to = size_;
    }
    return StringView(&data_[from], to - from);
}

bool StringView::StartsWith(const StringView& prefix) const {
    return Substr(0, prefix.Size()) == prefix;
}

bool StringView::EndsWith(const StringView& suffix) const {
    if (suffix.Size() > Size()) {
        return false;
    }
    return Substr(Size() - suffix.Size(), suffix.Size()) == suffix;
}

bool operator<(const StringView& l, const StringView& r) {
    return l.Compare(r) < 0;
}

bool operator>(const StringView& l, const StringView& r) {
    return r < l;
}

bool operator<=(const StringView& l, const StringView& r) {
    return !(r < l);
}

bool operator>=(const StringView& l, const StringView& r) {
    return r <= l;
}

bool operator==(const StringView& l, const StringView& r) {
    return l.Compare(r) == 0;
}

bool operator!=(const StringView& l, const StringView& r) {
    return !(l == r);
}

}  // namespace common
