#pragma once
#include <kernel/drivers/time/timer_driver.h>
#include <stdint.h>

namespace influx {
namespace time {
class time_manager {
   public:
    time_manager();

    double seconds() const;
    double milliseconds() const;

    void tick() const;

   private:
    drivers::timer_driver *_timer_driver;
};
};  // namespace time
};  // namespace influx