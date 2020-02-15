#pragma once
#include <stdint.h>

#include <interrupt_descriptor.h>
#include <kernel/interrupts/interrupt_service_routine.h>
#include <kernel/logger.h>

#define AMOUNT_OF_INTERRUPT_DESCRIPTORS 256
#define IDT_SIZE (sizeof(interrupt_descriptor_t) * AMOUNT_OF_INTERRUPT_DESCRIPTORS)

namespace influx {
namespace interrupts {
class interrupt_manager {
   public:
    interrupt_manager();

    void set_interrupt_service_routine(uint8_t interrupt_index, interrupt_service_routine isr);

   private:
    logger _log;
    interrupt_descriptor_t *_idt;

    void load_idt();
    void init_exception_interrupts();
};
};  // namespace interrupts
};  // namespace influx