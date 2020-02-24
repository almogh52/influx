#pragma once
#include <kernel/drivers/driver.h>

#include <kernel/drivers/ata/bus.h>
#include <kernel/drivers/ata/drive.h>
#include <kernel/drivers/ata/status_register.h>
#include <kernel/structures/string.h>
#include <kernel/structures/vector.h>

#define MAX_ATA_STRING_LENGTH 100

namespace influx {
namespace drivers {
class ata : public driver {
   public:
    ata();

    virtual void load();

    const structures::vector<ata_drive> drives() const { return _drives; };

   private:
    structures::vector<ata_drive> _drives;

    void detect_drives();

    void delay(const ata_bus &controller) const;

    structures::string ata_get_string(char *base, uint64_t size);
    ata_drive identify_drive(const ata_bus &controller, bool slave);

    ata_status_register read_status_register(const ata_bus &controller) const;
    ata_status_register read_status_register_with_mask(const ata_bus &controller, uint8_t mask,
                                                       uint8_t value,
                                                       uint64_t amount_of_tries) const;
};
};  // namespace drivers
};  // namespace influx