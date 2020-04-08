#pragma once
#include <kernel/drivers/time/timer_driver.h>

#include <kernel/interrupts/interrupt_regs.h>

#define PIT_IRQ 0

#define PIT_CHANNEL_0_PORT 0x40
#define PIT_COMMAND_PORT 0x43

#define PIT_MODE 0b00110110

#define PIT_REAL_FREQUENCY 1193182
#define PIT_FREQUENCY 1000

namespace influx {
namespace drivers {
class pit : public timer_driver {
   public:
    pit();

    virtual bool load();

    virtual uint64_t current_count() const { return _count; };
    virtual uint64_t count_frequency() const { return PIT_FREQUENCY; };

    virtual uint64_t timer_frequency() const { return PIT_FREQUENCY; }

	friend void pit_irq(influx::interrupts::regs *context, pit *pit);

   private:
    uint64_t _count;
};

void pit_irq(influx::interrupts::regs *context, pit *pit);
};  // namespace drivers
};  // namespace influx