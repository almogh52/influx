#include <kernel/threading/spinlock.h>

influx::threading::spinlock::spinlock() : _value(0) {}

void influx::threading::spinlock::lock() {
    while (!__sync_bool_compare_and_swap(&_value, 0, 1)) {
        __sync_synchronize();
    }
}

bool influx::threading::spinlock::try_lock() {
    // Try to lock without sync
    if (!__sync_bool_compare_and_swap(&_value, 0, 1)) {
        __sync_synchronize();
        if (!__sync_bool_compare_and_swap(&_value, 0, 1)) {
            return false;
        }
    }

    return true;
}

void influx::threading::spinlock::unlock() {
    __sync_synchronize();
    _value = 0;
}