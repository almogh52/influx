#pragma once
#include <kernel/interrupts/interrupt_service_routine.h>
#include <stdint.h>

namespace influx {
namespace interrupts {
void exception_interrupt_handler(struct interrupt_frame *frame, uint64_t error_code);
};
};  // namespace influx