#pragma once
#include <kernel/drivers/ata/constants.h>
#include <stdint.h>

namespace influx {
namespace drivers {
namespace ata {
struct bus {
    uint16_t io_base;
    uint16_t control_base;
};

inline bool operator==(const bus& a, const bus& b) {
    return a.io_base == b.io_base && a.control_base == b.control_base;
}

constexpr bus ata_primary_bus{.io_base = PRIMARY_ATA_BUS_IO_BASE,
                              .control_base = PRIMARY_ATA_BUS_CONTROL_BASE};
constexpr bus ata_secondary_bus{.io_base = SECONDARY_ATA_BUS_IO_BASE,
                                .control_base = SECONDARY_ATA_BUS_CONTROL_BASE};
};  // namespace ata
};  // namespace drivers
};  // namespace influx