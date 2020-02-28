#pragma once
#include <stdint.h>

#include <interrupt_descriptor.h>
#include <kernel/interrupts/interrupt_request.h>
#include <kernel/interrupts/interrupt_service_routine.h>
#include <kernel/logger.h>

#define AMOUNT_OF_INTERRUPT_DESCRIPTORS 256
#define IDT_SIZE (sizeof(interrupt_descriptor_t) * AMOUNT_OF_INTERRUPT_DESCRIPTORS)

#define PIC1_INTERRUPTS_OFFSET 32
#define PIC2_INTERRUPTS_OFFSET 40

#define PIC_INTERRUPT_COUNT 16

namespace influx {
namespace interrupts {
template <uint8_t number>
void irq_interrupt_handler(struct interrupt_frame *frame);

class interrupt_manager {
   public:
    interrupt_manager();

    void set_interrupt_service_routine(uint8_t interrupt_index, interrupt_service_routine isr);
    void set_irq_handler(uint8_t irq, uint64_t irq_handler_address, void *irq_handler_data);

    void enable_interrupts() const;
    void disable_interrupts() const;

    template <uint8_t number>
    friend void irq_interrupt_handler(struct interrupt_frame *frame);

   private:
    logger _log;

    interrupt_descriptor_t *_idt;
    interrupt_request _irqs[PIC_INTERRUPT_COUNT] = {0};

    void load_idt();
    void remap_pic_interrupts();

    void register_exception_interrupts();
    void register_pic_interrupts();
};
};  // namespace interrupts
};  // namespace influx