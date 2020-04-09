#pragma once
#include <kernel/structures/dynamic_buffer.h>

namespace influx {
struct segment {
    uint64_t virtual_address;
    structures::dynamic_buffer data;
    struct {
        bool read;
        bool write;
        bool execute;
    } permissions;
};
};  // namespace influx