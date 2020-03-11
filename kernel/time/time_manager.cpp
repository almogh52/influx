#include <kernel/time/time_manager.h>

#include <kernel/assert.h>
#include <kernel/kernel.h>

influx::time::time_manager::time_manager()
    : _timer_driver((drivers::timer_driver *)kernel::driver_manager()->get_driver("PIT")) {
    kassert(_timer_driver != nullptr);
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

void influx::time::time_manager::tick() const {
    // Call all tick handlers
    for (auto handler : _tick_handlers) {
        handler.function(handler.data);
    }
}

void influx::time::time_manager::register_tick_handler(void (*handler)(void *), void *data) {
    _tick_handlers.push_back(tick_handler{.function = handler, .data = data});
}