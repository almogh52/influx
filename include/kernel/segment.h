#pragma once
#include <memory/protection_flags.h>
#include <stdint.h>

namespace influx {
struct segment {
    uint64_t virtual_address;
    uint64_t size;
    protection_flags_t protection;
};
};  // namespace influx