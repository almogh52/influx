#pragma once
#include <interrupt_descriptor.h>

namespace influx {
namespace interrupts {
enum interrupt_service_routine_type {
    interrupt_gate = INTERRUPT_GATE_TYPE,
    trap_gate = TRAP_GATE_TYPE
};

struct interrupt_service_routine {
    interrupt_service_routine_type type;
    uint64_t routine_address;
};
};  // namespace interrupts
};  // namespace influx