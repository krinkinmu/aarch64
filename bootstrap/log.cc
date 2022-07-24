#include "bootstrap/log.h"
#include "bootstrap/pl011.h"
#include "util/algorithms.h"

namespace {

PL011* global_sink = nullptr;

}  // namespace

LogSink LogSink::Sink() {
    return LogSink(global_sink);
}

LogSink LogSink::Hole() {
    return LogSink(nullptr);
}

LogSink::LogSink(PL011* serial) : serial_(serial) {}

void LogSink::Log(util::StringView str) {
    if (serial_ == nullptr) {
        return;
    }
    serial_->Send(reinterpret_cast<const uint8_t*>(str.Data()), str.Size());
}

void SetupLogger() {
    // For HiKey960 board that I have the following parameters were found to        // work fine:
    //
    // PL011::Serial(
    //     /* base_address = */0xfff32000, /* base_clock = */19200000);
    static PL011 serial = PL011::Serial(0x9000000, 24000000);
    global_sink = &serial;
}

LogSink Log() {
    return LogSink::Sink();
}

LogSink operator<<(LogSink sink, util::StringView str) {
    sink.Log(str);
    return sink;
}

LogSink operator<<(LogSink sink, uintmax_t num) {
    if (num == 0) {
        return (sink << "0");
    }

    char buf[32];
    size_t size;

    for (size = 0; num != 0; ++size, num /= 10) {
        buf[size] = '0' + (num % 10);
    }

    for (size_t l = 0, r = size - 1; l < r; ++l, --r) {
        util::Swap(buf[l], buf[r]);
    }
    buf[size] = '\0';

    return (sink << buf);
}

LogSink operator<<(LogSink sink, intmax_t num) {
    if (num < 0) {
        return (sink << "-" << static_cast<uintmax_t>(-num));
    }
    return (sink << static_cast<uintmax_t>(num));
}

LogSink operator<<(LogSink sink, const void* ptr) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    if (addr == 0) {
        return (sink << "0x0");
    }

    char buf[32] = "0x";
    size_t size = 2;

    for (; addr > 0; ++size, addr >>= 4) {
        if ((addr & 0xf) < 10) {
            buf[size] = '0' + (addr & 0xf);
        } else {
            buf[size] = 'a' + (addr & 0xf) - 10;
        }
    }

    for (size_t l = 2, r = size - 1; l < r; ++l, --r) {
        util::Swap(buf[l], buf[r]);
    }
    buf[size] = '\0';
    return (sink << buf);
}

LogSink operator<<(LogSink sink, const char* str) {
    return (sink << util::StringView(str));
}
