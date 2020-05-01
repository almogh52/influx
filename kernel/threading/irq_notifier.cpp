#include <kernel/threading/irq_notifier.h>

#include <kernel/kernel.h>
#include <kernel/threading/interrupts_lock.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/scheduler.h>
#include <kernel/threading/unique_lock.h>

#include <kernel/console/console.h>

influx::threading::irq_notifier::irq_notifier() : _notified(false), _task(nullptr) {}

void influx::threading::irq_notifier::notify() noexcept {
    interrupts_lock lk;

    // If the wait queue is empty, set notified as true
    if (_task == nullptr) {
        _notified = true;
    } else if (_task != kernel::scheduler()->get_current_task()) {
        // Unblock the waiting task
        lk.unlock();
        kernel::scheduler()->unblock_task(_task);

        // Set the waiting task as null
        lk.lock();
        _task = nullptr;
    } else {
        _task->value().reblock_after_isr = false;
        _task = nullptr;
    }
}

void influx::threading::irq_notifier::wait() {
    interrupts_lock lk;

    // If the IRQ didn't notify yet, wait for notify
    if (!_notified) {
        // Save the waiting task
        _task = kernel::scheduler()->get_current_task();

        // Unlock interrupts and block the current task
        kernel::scheduler()->block_current_task();
        lk.unlock();
    } else {
        _notified = false;
    }
}

bool influx::threading::irq_notifier::wait_interruptible() {
    interrupts_lock lk;

    // If the IRQ didn't notify yet, wait for notify
    if (!_notified) {
        // Save the waiting task
        _task = kernel::scheduler()->get_current_task();

        // Set the task as interruptible
        _task->value().signal_interruptible = true;

        // Unlock interrupts and block the current task
        kernel::scheduler()->block_current_task();
        lk.unlock();

        // Set the task as uninterruptible
        kernel::scheduler()->get_current_task()->value().signal_interruptible = false;

        // If the task was interrupted by a signal return false
        if (kernel::scheduler()->get_current_task()->value().signal_interrupted) {
            return false;
        }
    } else {
        _notified = false;
    }

    return true;
}