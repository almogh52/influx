#include <kernel/drivers/time/pit.h>

#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/kernel.h>
#include <kernel/ports.h>

void influx::drivers::pit_irq(influx::interrupts::regs *context, influx::drivers::pit *pit) {
    // Increase the counter
    pit->_count++;

    // Tick the time manager
    if (kernel::time_manager() != nullptr) {
        kernel::time_manager()->tick();
    }
}

influx::drivers::pit::pit() : timer_driver("PIT"), _count(0) {}

bool influx::drivers::pit::load() {
    uint64_t frequency_divisor = PIT_REAL_FREQUENCY / PIT_FREQUENCY;

    // Register IRQ handler
    _log("Registering IRQ handler..\n");
    kernel::interrupt_manager()->set_irq_handler(PIT_IRQ, (uint64_t)influx::drivers::pit_irq, this);

    // Set the PIT to square wave generator mode and (lobyte/hibyte) access mode
    _log("Setting PIT mode and frequency divisor..\n");
    ports::out<uint8_t>(PIT_MODE, PIT_COMMAND_PORT);

    // Send the frequency divisor to the PIT
    ports::out<uint8_t>((uint8_t)frequency_divisor, PIT_CHANNEL_0_PORT);
    ports::out<uint8_t>((uint8_t)(frequency_divisor >> 8), PIT_CHANNEL_0_PORT);

    return true;
}