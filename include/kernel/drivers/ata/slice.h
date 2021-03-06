#pragma once
#include <kernel/drivers/ata/ata.h>
#include <kernel/drivers/ata/drive.h>
#include <stdint.h>

namespace influx {
namespace drivers {
namespace ata {
class drive_slice {
   public:
    drive_slice();
    drive_slice(ata *driver, const drive &drive, uint32_t start_lba);

    uint64_t read(uint64_t address, uint64_t amount, void *buffer, bool interruptible = false);
    uint64_t write(uint64_t address, uint64_t amount, void *data, bool interruptible = false);

    const drive &drive_info() const { return _drive; };

   private:
    ata *_driver;

    drive _drive;
    uint32_t _start_lba;
};
};  // namespace ata
};  // namespace drivers
};  // namespace influx