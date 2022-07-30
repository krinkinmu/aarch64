#ifndef __COMMON_STREAM_H__
#define __COMMON_STREAM_H__

namespace common {

/*
 * OutputStream is a simple interface that represents something we can write
 * into. It's not intended to be used directly, instead specific
 * implementations are expected to inherit it and implement the virtual methods.
 *
 * TODO: for IO interfaces like that we should also define how errors are
 *       reported, so far we expect errors to be reported as negative return
 *       values of the functions.
 */
class OutputStream {
public:
    virtual ~OutputStream();

    /*
     * Put writes character @c to the OutputStream.
     * When successful returns non-negative value, otherwise returns a negative
     * value.
     */
    virtual int Put(char c) = 0;

    /*
     * PutN writes @n characters from @data to the OutputStream.
     * If the function successfully writes anything to the OutputStream it
     * should return the number of characters it wrote. If the function failed
     * to write anything, it should return a negative value. In all other cases
     * (when @n is zero), it should return zero.
     */
    virtual int PutN(const char* data, int n);
};

OutputStream& operator<<(OutputStream& out, char c);
OutputStream& operator<<(OutputStream& out, int x);
OutputStream& operator<<(OutputStream& out, unsigned x);
OutputStream& operator<<(OutputStream& out, long x);
OutputStream& operator<<(OutputStream& out, unsigned long x);
OutputStream& operator<<(OutputStream& out, long long x);
OutputStream& operator<<(OutputStream& out, unsigned long long x);
OutputStream& operator<<(OutputStream& out, const char* str);
OutputStream& operator<<(OutputStream& out, const void* ptr);

}  // namespace common

#endif  // __COMMON_STREAM_H__
