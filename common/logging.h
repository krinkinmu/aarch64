#ifndef __COMMON_LOGGING_H__
#define __COMMON_LOGGING_H__

#include "common/stream.h"

namespace common {

void RegisterLog(OutputStream* out);
OutputStream& Log();

}  // namespace common

#endif  // __COMMON_LOGGING_H__
