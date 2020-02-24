#pragma once
#include <stdint.h>

namespace influx {
namespace drivers {
struct ata_status_register {
    union {
        uint8_t raw;
        struct {
            uint8_t err : 1;
            uint8_t idx : 1;
            uint8_t corr : 1;
            uint8_t drq : 1;
            uint8_t srv : 1;
            uint8_t df : 1;
            uint8_t rdy : 1;
            uint8_t bsy : 1;
        };
    };
};
};  // namespace drivers
};  // namespace influx