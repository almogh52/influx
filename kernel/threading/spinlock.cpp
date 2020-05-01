#include <kernel/threading/spinlock.h>

#include <kernel/kernel.h>

influx::threading::spinlock::spinlock() : _value(0) {}

void influx::threading::spinlock::lock() {
    while (!__sync_bool_compare_and_swap(&_value, 0, 1))
        ;
    __sync_synchronize();
}

bool influx::threading::spinlock::try_lock() {
    // Try to lock without sync
    if (__sync_bool_compare_and_swap(&_value, 0, 1)) {
        __sync_synchronize();
        return true;
    }

    return false;
}

void influx::threading::spinlock::unlock() {
    __sync_synchronize();
    _value = 0;
}