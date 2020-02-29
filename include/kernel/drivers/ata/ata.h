#pragma once
#include <kernel/drivers/driver.h>

#include <kernel/drivers/ata/access_type.h>
#include <kernel/drivers/ata/bus.h>
#include <kernel/drivers/ata/drive.h>
#include <kernel/drivers/ata/status_register.h>
#include <kernel/interrupts/interrupt_service_routine.h>
#include <kernel/structures/string.h>
#include <kernel/structures/vector.h>

#define MAX_ATA_STRING_LENGTH 100

#define MAX_ATA_SECTOR_ACCESS 256

namespace influx {
namespace drivers {
namespace ata {
class ata : public driver {
   public:
    ata();

    virtual void load();

    bool access_drive_sectors(const drive &drive, access_type access_type, uint32_t lba,
                              uint16_t amount_of_sectors, uint16_t *data);

    const structures::vector<drive> drives() const { return _drives; };

    friend void primary_irq(influx::interrupts::interrupt_frame *frame, ata *ata);
    friend void secondary_irq(influx::interrupts::interrupt_frame *frame, ata *ata);

   private:
    drive _selected_drive;
    structures::vector<drive> _drives;

    uint32_t _primary_irq_called, _secondary_irq_called;

    void wait_for_primary_irq();
    void wait_for_secondary_irq();

    void detect_drives();

    bool select_drive(const drive &drive);
    void delay(const bus &controller) const;

    structures::string ata_get_string(char *base, uint64_t size);
    drive identify_drive(const bus &controller, bool slave);

    status_register read_status_register(const bus &controller) const;
    status_register read_status_register_with_mask(const bus &controller, uint8_t mask,
                                                   uint8_t value, uint64_t amount_of_tries) const;
};

void primary_irq(influx::interrupts::interrupt_frame *frame, ata *ata);
void secondary_irq(influx::interrupts::interrupt_frame *frame, ata *ata);
};  // namespace ata
};  // namespace drivers
};  // namespace influx