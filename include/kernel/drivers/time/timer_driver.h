#pragma once
#include <kernel/drivers/driver.h>
#include <stdint.h>

namespace influx {
namespace drivers {
class timer_driver : public driver {
   public:
    timer_driver(const structures::string& driver_name) : driver(driver_name) {}
    virtual ~timer_driver(){};

    virtual uint64_t current_count() const = 0;
    virtual uint64_t count_frequency() const = 0;

    virtual uint64_t timer_frequency() const = 0;
};
};  // namespace drivers
};  // namespace influx