#ifndef __COMMON_STRING_VIEW_H__
#define __COMMON_STRING_VIEW_H__

#include <stddef.h>


namespace common {

class StringView {
public:
    StringView();
    StringView(const char* cstr);
    StringView(const char* data, size_t size);

    StringView(const StringView& other) = default;
    StringView& operator=(const StringView& other) = default;


    size_t Size() const;
    size_t Length() const;
    bool Empty() const;

    char operator[](size_t pos) const;
    const char *Data() const;

    int Compare(const StringView& other) const;

    StringView Substr(size_t from, size_t count) const;

    bool StartsWith(const StringView& prefix) const;
    bool EndsWith(const StringView& suffix) const;
    bool Contains(const StringView& substr) const;

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

}  // namespace common

#endif  // __COMMON_STRING_VIEW_H__
