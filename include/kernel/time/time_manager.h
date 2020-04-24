#pragma once
#include <kernel/drivers/time/cmos.h>
#include <kernel/drivers/time/timer_driver.h>
#include <kernel/structures/vector.h>
#include <kernel/time/timeval.h>
#include <stdint.h>

namespace influx {
namespace time {
struct tick_handler {
    void (*function)(void *);
    void *data;
};

class time_manager {
   public:
    time_manager();

    double seconds() const;
    double milliseconds() const;

    uint64_t unix_timestamp() const;
    uint64_t unix_timestamp_ms() const;
    timeval get_timeval() const;

    uint64_t timer_frequency() const;

    void tick();
    void register_tick_handler(void (*handler)(void *), void *data);

   private:
    drivers::timer_driver *_timer_driver;
    drivers::cmos *_cmos_driver;

    double _unix_timestamp;

    structures::vector<tick_handler> _tick_handlers;
};
};  // namespace time
};  // namespace influx