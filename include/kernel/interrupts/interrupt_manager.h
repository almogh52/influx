#pragma once
#include <kernel/logger.h>
#include <interrupt_descriptor.h>

#define AMOUNT_OF_INTERRUPT_DESCRIPTORS 256
#define IDT_SIZE (sizeof(interrupt_descriptor_t) * AMOUNT_OF_INTERRUPT_DESCRIPTORS)

namespace influx {
namespace interrupts {
class interrupt_manager {
   public:
    interrupt_manager();

   private:
    logger _log;
    interrupt_descriptor_t *_idt;

    void load_idt();

};
};  // namespace interrupts
};  // namespace influx