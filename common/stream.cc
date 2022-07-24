#include "common/stream.h"

namespace common {

OutputStream::~OutputStream() {}

int OutputStream::PutN(const char* data, int n) {
    for (int i = 0; i < n; ++i) {
        const int ret = this->Put(data[i]);
        if (ret < 0) {
            if (i > 0) {
                return i;
            }
            return ret;
        }
    }
    return n;
}

}  // namespace common

