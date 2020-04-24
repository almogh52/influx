#pragma once
#include <stdint.h>

namespace influx {
namespace time {
struct timeval {
    uint64_t seconds;
    uint64_t useconds;
};
};  // namespace time
};  // namespace influx