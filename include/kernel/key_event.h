#pragma once
#include <kernel/key_code.h>
#include <stdint.h>

namespace influx {
struct key_event {
    uint8_t raw_key;
    key_code code;
    bool released;
};
};  // namespace influx