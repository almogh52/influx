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

    // If the new queue head is the old queue head, set the queue head as null
    if (_queue_head == current_head) {
        _queue_head = nullptr;
    }

    // Unblock the task
    kernel::scheduler()->unblock_task(task);

    return task;
}

influx::structures::vector<influx::threading::tcb *>
influx::threading::task_wait_queue::dequeue_all() {
    lock_guard<spinlock> lk(_queue_lock);
    kassert(_queue_head != nullptr);

    structures::vector<tcb *> tasks;
    structures::node<tcb *> *current_task;

    // Make the list a flat list
    _queue_head->prev()->next() = nullptr;

    // While we didn't reach the end of the list
    while (_queue_head != nullptr) {
        // Add the task to the vector of tasks
        tasks += _queue_head->value();

        // Unblock the task
        kernel::scheduler()->unblock_task(_queue_head->value());

        // Move to the next task
        current_task = _queue_head;
        _queue_head = _queue_head->next();

        // Free the current task node
        delete current_task;
    }

    return tasks;
}

void influx::threading::task_wait_queue::remove_task(influx::threading::tcb *task) {
    lock_guard<spinlock> lk(_queue_lock);

    structures::node<tcb *> *current_node = _queue_head;

    // If the queue is empty
    if (_queue_head == nullptr) {
        return;
    }

    // Find the task's node
    do {
        // If it's the task's node, exit loop
        if (current_node->value() == task) {
            break;
        }

        // Move to the next node
        current_node = current_node->next();
    } while (current_node != _queue_head);

    // If we found the task's node
    if (current_node->value() == task) {
        // If the node is the head, change the head to next node
        if (_queue_head == current_node && _queue_head->next() != current_node) {
            _queue_head = _queue_head->next();
        } else if (_queue_head == current_node) {
            _queue_head = nullptr;
        }

        // Delete and free the node
        current_node->prev()->next() = current_node->next();
        current_node->next()->prev() = current_node->prev();
        delete current_node;
    }
}

bool influx::threading::task_wait_queue::empty() {
    lock_guard<spinlock> lk(_queue_lock);

    return _queue_head == nullptr;
}