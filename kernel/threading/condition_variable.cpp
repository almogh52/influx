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

bool influx::threading::condition_variable::wait_interruptible(
    influx::threading::unique_lock<influx::threading::mutex> &lock) {
    // Set the task as interruptible
    kernel::scheduler()->get_current_task()->value().signal_interruptible = true;

    // Add the current thread to task queue list and block it
    _wait_queue.enqueue(kernel::scheduler()->get_current_task());

    // Unlock the thread's lock
    lock.unlock();

    // Reschedule to another task
    kernel::scheduler()->reschedule();

    // Set the task as uninterruptible
    kernel::scheduler()->get_current_task()->value().signal_interruptible = false;

    // If the task was interrupted by a signal return false
    if (kernel::scheduler()->get_current_task()->value().signal_interrupted) {
        // Remove the task from the queue if it's in the queue
        _wait_queue.remove_task(kernel::scheduler()->get_current_task());

        return false;
    }

    // Reacquire the lock
    lock.lock();

    return true;
}