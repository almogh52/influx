#include <kernel/console/console.h>
#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/kernel.h>
#include <kernel/pic.h>
#include <kernel/ports.h>

template <uint8_t number>
__attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler(
    influx::interrupts::interrupt_frame *frame) {
    // Check if there is a handler for this IRQ
    if (influx::kernel::interrupt_manager()->_irqs[number].handler_address != 0) {
        ((void (*)(influx::interrupts::interrupt_frame *,
                   void *))influx::kernel::interrupt_manager()
             ->_irqs[number]
             .handler_address)(frame,
                               influx::kernel::interrupt_manager()->_irqs[number].handler_data);
    }

    // If the IRQ is on the slave PIC, send EOI to it
    if (number >= 8) {
        influx::ports::out<uint8_t>(PIC_EOI, PIC2_COMMAND);
    }

    // Send EOI to the master PIC
    influx::ports::out<uint8_t>(PIC_EOI, PIC1_COMMAND);
}

template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<0>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<1>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<2>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<3>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<4>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<5>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<6>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<7>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<8>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<9>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<10>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<11>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<12>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<13>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<14>(
    influx::interrupts::interrupt_frame *frame);
template __attribute__((interrupt)) void influx::interrupts::irq_interrupt_handler<15>(
    influx::interrupts::interrupt_frame *frame);