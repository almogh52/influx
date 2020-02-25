#pragma once
#include <kernel/drivers/ata/bus.h>
#include <kernel/structures/string.h>
#include <stdint.h>

namespace influx {
namespace drivers {
struct ata_drive {
    bool present;
    ata_bus controller;
    bool slave;
    uint64_t size;
    structures::string serial_number;
    structures::string model_number;
    structures::string firmware_revision;
};

inline bool operator==(const ata_drive& a, const ata_drive& b) {
    return a.present && b.present && a.controller == b.controller && a.slave == b.slave;
};
};  // namespace drivers
};  // namespace influx