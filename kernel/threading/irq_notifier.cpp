#include <kernel/threading/irq_notifier.h>

#include <kernel/kernel.h>
#include <kernel/threading/interrupts_lock.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/scheduler.h>
#include <kernel/threading/unique_lock.h>

#include <kernel/console/console.h>

influx::threading::irq_notifier::irq_notifier() : _notified(false), _task(nullptr) {}

void influx::threading::irq_notifier::notify() noexcept {
    // Interrupts should be disabled by ISR

    // If the wait queue is empty, set notified as true
    if (_task == nullptr) {
        _notified = true;
    } else if (_task != kernel::scheduler()->get_current_task()) {
        kernel::scheduler()->unblock_task(_task);
        _task = nullptr;
    } else {
		_task->value().state = thread_state::running;
		_task = nullptr;
	}
}

void influx::threading::irq_notifier::wait() {
    interrupts_lock lk;

    // If the IRQ didn't notify yet, wait for notify
    if (!_notified) {
        _task = kernel::scheduler()->get_current_task();
        kernel::scheduler()->block_task(_task);
        lk.unlock();
    } else {
        _notified = false;
    }
}