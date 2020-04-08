#pragma once
#include <stdint.h>

#include <interrupt_descriptor.h>
#include <kernel/interrupts/interrupt_regs.h>
#include <kernel/interrupts/interrupt_request.h>
#include <kernel/logger.h>

#define AMOUNT_OF_INTERRUPT_DESCRIPTORS 256
#define IDT_SIZE (sizeof(interrupt_descriptor_t) * AMOUNT_OF_INTERRUPT_DESCRIPTORS)

#define PIC1_INTERRUPTS_OFFSET 32
#define PIC2_INTERRUPTS_OFFSET 40

#define PIC_INTERRUPT_COUNT 16

namespace influx {
namespace interrupts {
enum class interrupt_service_routine_type {
    interrupt_gate = INTERRUPT_GATE_TYPE,
    trap_gate = TRAP_GATE_TYPE
};

extern "C" void isr_handler(regs *context);

void exception_interrupt_handler(regs *context);
void irq_interrupt_handler(regs *context);

class interrupt_manager {
   public:
    interrupt_manager();

    void set_interrupt_service_routine(uint8_t interrupt_index, uint64_t isr);
    void set_irq_handler(uint8_t irq, uint64_t irq_handler_address, void *irq_handler_data);

    void enable_interrupts() const;
    void disable_interrupts() const;

    friend void irq_interrupt_handler(regs *frame);

   private:
    logger _log;

    interrupt_descriptor_t *_idt;
    interrupt_request _irqs[PIC_INTERRUPT_COUNT] = {0};

    void init_isrs();
    void set_isr(uint8_t interrupt_index, uint64_t isr, interrupt_service_routine_type type);

    void load_idt();
    void remap_pic_interrupts();

    void register_exception_interrupts();
    void register_pic_interrupts();
};
};  // namespace interrupts
};  // namespace influx