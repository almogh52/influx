#include <kernel/threading/task_wait_queue.h>

#include <kernel/assert.h>
#include <kernel/kernel.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/scheduler.h>

influx::threading::task_wait_queue::task_wait_queue() : _queue_head(nullptr) {}

void influx::threading::task_wait_queue::enqueue(influx::threading::tcb *task) {
    lock_guard<spinlock> lk(_queue_lock);

    // If the queue is empty, create the first node
    if (_queue_head == nullptr) {
        _queue_head = new structures::node<tcb *>(task);
        _queue_head->next() = _queue_head;
        _queue_head->prev() = _queue_head;
    } else {
        _queue_head->prev()->next() =
            new structures::node<tcb *>(task, _queue_head->prev(), _queue_head);
        _queue_head->prev() = _queue_head->prev()->next();
    }

    // Block the task
    kernel::scheduler()->block_task(task);
}

influx::threading::tcb *influx::threading::task_wait_queue::dequeue() {
    lock_guard<spinlock> lk(_queue_lock);
    kassert(_queue_head != nullptr);

    structures::node<tcb *> *current_head = _queue_head;
    tcb *task = _queue_head->value();

    // Delete and free the node
    current_head->prev()->next() = current_head->next();
    current_head->next()->prev() = current_head->prev();
    _queue_head = current_head->next();
    delete current_head;

    // Unblock the task
    kernel::scheduler()->unblock_task(task);

    return task;
}

bool influx::threading::task_wait_queue::empty() const { return _queue_head == nullptr; }