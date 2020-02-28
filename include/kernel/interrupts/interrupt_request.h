#pragma once

namespace influx {
namespace interrupts {
struct interrupt_request {
    uint64_t handler_address;
    void *handler_data;
};
};  // namespace interrupts
};  // namespace influx