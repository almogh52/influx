#pragma once
#include <kernel/structures/dynamic_buffer.h>
#include <memory/protection_flags.h>

namespace influx {
struct segment {
    uint64_t virtual_address;
    structures::dynamic_buffer data;
    protection_flags_t protection;
};
};  // namespace influx