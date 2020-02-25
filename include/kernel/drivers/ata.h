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

namespace influx {
namespace drivers {
class ata : public driver {
   public:
    ata();

    virtual void load();

    const structures::vector<ata_drive> drives() const { return _drives; };

    friend void ata_primary_irq(influx::interrupts::interrupt_frame *frame, ata *ata);
    friend void ata_secondary_irq(influx::interrupts::interrupt_frame *frame, ata *ata);

   private:
    ata_drive _selected_drive;
    structures::vector<ata_drive> _drives;

    uint32_t _primary_irq_called, _secondary_irq_called;

    void wait_for_primary_irq();
    void wait_for_secondary_irq();

    void detect_drives();

    bool access_drive_sectors(const ata_drive &drive, ata_access_type access_type, uint32_t lba,
                              uint16_t amount_of_sectors, uint16_t *data);

    bool select_drive(const ata_drive &drive);
    void delay(const ata_bus &controller) const;

    structures::string ata_get_string(char *base, uint64_t size);
    ata_drive identify_drive(const ata_bus &controller, bool slave);

    ata_status_register read_status_register(const ata_bus &controller) const;
    ata_status_register read_status_register_with_mask(const ata_bus &controller, uint8_t mask,
                                                       uint8_t value,
                                                       uint64_t amount_of_tries) const;
};

void ata_primary_irq(influx::interrupts::interrupt_frame *frame, ata *ata);
void ata_secondary_irq(influx::interrupts::interrupt_frame *frame, ata *ata);
};  // namespace drivers
};  // namespace influx