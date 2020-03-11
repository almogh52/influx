#include <kernel/threading/interrupts_lock.h>

#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/kernel.h>

influx::threading::interrupts_lock::interrupts_lock() {
    // Disable interrupts when object initialized
    kernel::interrupt_manager()->disable_interrupts();
}

influx::threading::interrupts_lock::~interrupts_lock() {
    // Re-enable interrupts when object is destroyed
    kernel::interrupt_manager()->enable_interrupts();
}