#include "common/logging.h"

namespace common {

namespace {

struct NoopSink final : public OutputStream {
    int Put(char) override { return 0; }
    int PutN(const char*, int n) override { return n; }
};

OutputStream* logging_sink = nullptr;

}  // namespace

void RegisterLog(OutputStream* out) {
    logging_sink = out;
}

OutputStream& Log() {
    static NoopSink noop_sink;
    if (logging_sink == nullptr) {
        return noop_sink;
    }
    return *logging_sink;
}

}  // namespace common

