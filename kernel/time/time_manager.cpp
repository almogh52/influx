#include <kernel/time/time_manager.h>

#include <kernel/assert.h>
#include <kernel/kernel.h>

influx::time::time_manager::time_manager()
    : _timer_driver((drivers::timer_driver *)kernel::driver_manager()->get_driver("PIT")) {}

double influx::time::time_manager::seconds() const {
    kassert(_timer_driver != nullptr);

    return (double)_timer_driver->current_count() / (double)_timer_driver->count_frequency();
}

double influx::time::time_manager::milliseconds() const {
    kassert(_timer_driver != nullptr);

    return (double)_timer_driver->current_count() / ((double)_timer_driver->count_frequency() / 1000.0);
}

void influx::time::time_manager::tick() const {}