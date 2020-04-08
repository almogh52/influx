#pragma once
#include <stdint.h>

namespace influx {
namespace vfs {
union file_permissions {
    uint8_t raw;
    struct {
        uint8_t read : 1;
        uint8_t write : 1;
        uint8_t execute : 1;
    };
};
};  // namespace vfs
};  // namespace influx