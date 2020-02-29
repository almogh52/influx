#include <kernel/drivers/ata/ata.h>

#include <kernel/console/console.h>

void influx::drivers::ata::primary_irq(influx::interrupts::interrupt_frame *frame,
                                       influx::drivers::ata::ata *ata) {
    // Notify the ATA driver for primary irq
    while (!__sync_bool_compare_and_swap(&ata->_primary_irq_called, 0, 1)) {
        __sync_synchronize();
    }
}

void influx::drivers::ata::secondary_irq(influx::interrupts::interrupt_frame *frame,
                                         influx::drivers::ata::ata *ata) {
    // Notify the ATA driver for secondary irq
    while (!__sync_bool_compare_and_swap(&ata->_secondary_irq_called, 0, 1)) {
        __sync_synchronize();
    }
}