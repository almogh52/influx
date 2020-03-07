#include <kernel/drivers/time/pit.h>

void influx::drivers::pit_irq(influx::interrupts::interrupt_frame *frame,
                              influx::drivers::pit *pit) {
    // Increase the counter
    pit->_count++;
}