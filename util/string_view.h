#ifndef __UTIL_STRING_VIEW_H__
#define __UTIL_STRING_VIEW_H__

#include "util/stddef.h"

namespace util {

class StringView {
public:
    StringView();
    StringView(const char* str);
    StringView(const char* data, size_t size);

    StringView(const StringView& other) = default;
    StringView& operator=(const StringView& other) = default;

    size_t Size() const;
    size_t Length() const;
    bool Empty() const;
    char operator[](size_t pos) const;
    const char *Data() const;
    int Compare(const StringView& other) const;
    StringView Substr(size_t from, size_t to) const;
    bool StartsWith(const StringView& prefix) const;
    bool EndsWith(const StringView& suffix) const; 

private:
    const char* data_;
    size_t size_;
};

bool operator==(const StringView& l, const StringView& r);
bool operator!=(const StringView& l, const StringView& r);
bool operator<(const StringView& l, const StringView& r);
bool operator<=(const StringView& l, const StringView& r);
bool operator>(const StringView& l, const StringView& r);
bool operator>=(const StringView& l, const StringView& r);

}  // namespace util

#endif  // __UTIL_STRING_VIEW_H__
