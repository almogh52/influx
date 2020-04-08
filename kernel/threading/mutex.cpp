#include <kernel/threading/mutex.h>

#include <kernel/kernel.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/scheduler.h>
#include <kernel/threading/unique_lock.h>

influx::threading::mutex::mutex() : _value(0) {}

void influx::threading::mutex::lock() {
    unique_lock lk(_value_lock);

    // If the mutex is unlocked, lock it
    if (_value == 0) {
        _value = 1;
    } else {
        // Unlock the value lock
        lk.unlock();

        // Add the task to the wait queue
        _wait_queue.enqueue(kernel::scheduler()->get_current_task());

        // Reschedule to another task
        kernel::scheduler()->reschedule();
    }
}

bool influx::threading::mutex::try_lock() {
    lock_guard lk(_value_lock);

    // If the mutex is unlocked, lock it
    if (_value == 0) {
        _value = 1;

        return true;
    }

    return false;
}

void influx::threading::mutex::unlock() {
    unique_lock lk(_value_lock, defer_lock);

    // Dequeue task from waiting list if not empty
    if (!_wait_queue.empty()) {
        _wait_queue.dequeue();
    } else {
        // Unlock the mutex
        lk.lock();
        _value = 0;
    }
}