#include <kernel/threading/condition_variable.h>

#include <kernel/kernel.h>
#include <kernel/threading/scheduler.h>

influx::threading::condition_variable::condition_variable() {}

void influx::threading::condition_variable::notify_one() noexcept {
    // If the wait queue isn't empty, dequeue a task and unblock it
    if (!_wait_queue.empty()) {
        _wait_queue.dequeue();
    }
}

void influx::threading::condition_variable::notify_all() noexcept {
    // If the wait queue isn't empty, dequeue all tasks and unblock them
    if (!_wait_queue.empty()) {
        _wait_queue.dequeue_all();
    }
}

void influx::threading::condition_variable::wait(
    influx::threading::unique_lock<influx::threading::mutex> &lock) {
    // Add the current thread to task queue list and block it
    _wait_queue.enqueue(kernel::scheduler()->get_current_task());

    // Unlock the thread's lock
    lock.unlock();

    // Reschedule to another task
    kernel::scheduler()->reschedule();

    // Reacquire the lock
    lock.lock();
}