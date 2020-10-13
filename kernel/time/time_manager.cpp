#include <kernel/time/time_manager.h>

#include <kernel/assert.h>
#include <kernel/kernel.h>
#include <kernel/threading/interrupts_lock.h>

influx::time::time_manager::time_manager()
    : _timer_driver((drivers::timer_driver *)kernel::driver_manager()->get_driver("PIT")),
      _cmos_driver((drivers::cmos *)kernel::driver_manager()->get_driver("CMOS")),
      _unix_timestamp(0),
      _tick_handler({nullptr, nullptr}) {
    kassert(_timer_driver != nullptr);
    kassert(_cmos_driver != nullptr);

    // Get unix timestamp
    _unix_timestamp = (double)_cmos_driver->get_unix_timestamp();
}

double influx::time::time_manager::seconds() const {
    kassert(_timer_driver != nullptr);

    return (double)_timer_driver->current_count() / (double)_timer_driver->count_frequency();
}

double influx::time::time_manager::milliseconds() const {
    kassert(_timer_driver != nullptr);

    return (double)_timer_driver->current_count() /
           ((double)_timer_driver->count_frequency() / 1000.0);
}

uint64_t influx::time::time_manager::unix_timestamp() const { return (uint64_t)_unix_timestamp; }

uint64_t influx::time::time_manager::unix_timestamp_ms() const {
    return (uint64_t)(_unix_timestamp * 1000);
}

influx::time::timeval influx::time::time_manager::get_timeval() const {
    return timeval{.seconds = unix_timestamp(), .useconds = unix_timestamp_ms() * 1000};
}

uint64_t influx::time::time_manager::timer_frequency() const {
    return _timer_driver->timer_frequency();
}

void influx::time::time_manager::tick() {
    // Update unix timestamp
    _unix_timestamp += 0.001;

    // Call tick handler
    if (_tick_handler.function != nullptr) {
        _tick_handler.function(_tick_handler.data);
    }
}

void influx::time::time_manager::register_tick_handler(void (*handler)(void *), void *data) {
    threading::interrupts_lock int_lk;

    _tick_handler = tick_handler{.function = handler, .data = data};
}