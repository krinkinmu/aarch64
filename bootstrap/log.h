#ifndef __LOG_H__
#define __LOG_H__

#include "bootstrap/pl011.h"
#include "util/string_view.h"
#include "util/stdint.h"
#include "util/stddef.h"


class LogSink {
public:
    static LogSink Sink();
    static LogSink Hole();

    LogSink(const LogSink& other) = default;
    LogSink& operator=(const LogSink& other) = delete;

    void Log(util::StringView str);

private:
    LogSink(PL011* serial);

    PL011* serial_;
};

void SetupLogger();
LogSink Log();

LogSink operator<<(LogSink sink, util::StringView str);
LogSink operator<<(LogSink sink, uintmax_t num);
LogSink operator<<(LogSink sink, intmax_t num);
LogSink operator<<(LogSink sink, const void* ptr);
LogSink operator<<(LogSink sink, const char* str);

template <typename T>
LogSink operator<<(LogSink sink, const T* ptr) {
    return (sink << static_cast<const void*>(ptr));
}

#endif  // __LOG_H__
