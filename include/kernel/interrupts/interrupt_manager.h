#pragma once
#include <stdint.h>

#include <interrupt_descriptor.h>
#include <kernel/interrupts/interrupt_regs.h>
#include <kernel/interrupts/interrupt_request.h>
#include <kernel/logger.h>
#include <kernel/threading/condition_variable.h>
#include <kernel/threading/mutex.h>
#include <kernel/threading/thread.h>

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
    void set_interrupt_privilege_level(uint8_t interrupt_index, uint8_t privilege_level);

    void set_irq_handler(uint8_t irq, uint64_t irq_handler_address, void *irq_handler_data);
    bool wait_for_irq(uint8_t irq, bool interruptible);

    void start_irq_notify_thread();

    void enable_interrupts() const;
    void disable_interrupts() const;

   private:
    logger _log;

    interrupt_descriptor_t *_idt;

    threading::tcb *_irq_notify_task;
    interrupt_request _irq_handlers[PIC_INTERRUPT_COUNT] = {0};
    uint64_t _irq_call_count[PIC_INTERRUPT_COUNT] = {0};
    uint32_t _irq_call_count_changed;
    threading::mutex _irq_mutexes[PIC_INTERRUPT_COUNT];
    threading::condition_variable _irq_cvs[PIC_INTERRUPT_COUNT];

    void init_isrs();
    void set_isr(uint8_t interrupt_index, uint64_t isr, interrupt_service_routine_type type);

    void load_idt();
    void remap_pic_interrupts();

    void register_exception_interrupts();
    void register_pic_interrupts();

    void irq_notify_thread();

    friend void irq_interrupt_handler(regs *frame);
};
};  // namespace interrupts
};  // namespace influx