#pragma once
#include <kernel/drivers/driver.h>

#include <kernel/drivers/ata/access_type.h>
#include <kernel/drivers/ata/bus.h>
#include <kernel/drivers/ata/drive.h>
#include <kernel/drivers/ata/status_register.h>
#include <kernel/interrupts/interrupt_regs.h>
#include <kernel/structures/string.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/irq_notifier.h>
#include <kernel/threading/mutex.h>

#define MAX_ATA_STRING_LENGTH 100

#define MAX_ATA_SECTOR_ACCESS 256

namespace influx {
namespace drivers {
namespace ata {
class ata : public driver {
   public:
    ata();

    virtual bool load();

    uint16_t access_drive_sectors(const drive &drive, access_type access_type, uint32_t lba,
                                  uint16_t amount_of_sectors, uint16_t *data,
                                  bool interruptible = false);

    const structures::vector<drive> drives() const { return _drives; };

    friend void primary_irq(influx::interrupts::regs *context, ata *ata);
    friend void secondary_irq(influx::interrupts::regs *context, ata *ata);

   private:
    threading::mutex _mutex;

    drive _selected_drive;
    structures::vector<drive> _drives;

    uint32_t _primary_irq_called, _secondary_irq_called;

    threading::irq_notifier _primary_irq_notifier, _secondary_irq_notifier;

    bool wait_for_primary_irq(bool interruptible);
    bool wait_for_secondary_irq(bool interruptible);

    void detect_drives();

    bool select_drive(const drive &drive);
    void delay(const bus &controller) const;

    structures::string ata_get_string(char *base, uint64_t size);
    drive identify_drive(const bus &controller, bool slave);

    status_register read_status_register(const bus &controller) const;
    status_register read_status_register_with_mask(const bus &controller, uint8_t mask,
                                                   uint8_t value, uint64_t amount_of_tries) const;
};

void primary_irq(influx::interrupts::regs *context, ata *ata);
void secondary_irq(influx::interrupts::regs *context, ata *ata);
};  // namespace ata
};  // namespace drivers
};  // namespace influx