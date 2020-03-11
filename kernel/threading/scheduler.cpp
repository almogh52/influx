#include <kernel/threading/scheduler.h>

#include <kernel/assert.h>
#include <kernel/kernel.h>
#include <kernel/threading/interrupts_lock.h>
#include <kernel/threading/scheduler_utils.h>
#include <kernel/time/time_manager.h>
#include <kernel/utils.h>

influx::threading::scheduler::scheduler()
    : _log("Scheduler", console_color::blue),
      _priority_queues(MAX_PRIORITY_LEVEL + 1),
      _current_task(nullptr),
      _max_quantum((kernel::time_manager()->timer_frequency() / 1000) * TASK_MAX_TIME_SLICE) {
    kassert(_max_quantum != 0);

    // Create kernel process
    _log("Creating kernel process..\n");
    _processes.insert_unique({.pid = 0,
                              .ppid = 0,
                              .priority = MAX_PRIORITY_LEVEL,
                              .system = true,
                              .cr3 = scheduler_utils::get_cr3(),
                              .kernel_stack = 0,
                              .name = "kernel"});

    // Create kernel main thread
    _log("Creating kernel main thread..\n");
    _priority_queues[MAX_PRIORITY_LEVEL].start = new tcb(thread{
        .tid = 0, .pid = 0, .context = nullptr, .state = thread_state::running, .quantum = 0});
    _priority_queues[MAX_PRIORITY_LEVEL].next_task = nullptr;
    _priority_queues[MAX_PRIORITY_LEVEL].start->prev() = _priority_queues[MAX_PRIORITY_LEVEL].start;
    _priority_queues[MAX_PRIORITY_LEVEL].start->next() = _priority_queues[MAX_PRIORITY_LEVEL].start;

    // Set the current task as the kernel main thread
    _current_task = _priority_queues[MAX_PRIORITY_LEVEL].start;

    // Register tick handler
    _log("Registering tick handler..\n");
    kernel::time_manager()->register_tick_handler(
        utils::method_function_wrapper<scheduler, &scheduler::tick_handler>, this);
}

void influx::threading::scheduler::reschedule() {
    interrupts_lock int_lk;

    tcb *current_task = _current_task;
    tcb *next_task = get_next_task();

    // If there isn't a next task and the current task isn't blocked, keep running it
    if (next_task == nullptr && current_task->value().state == thread_state::running) {
        next_task = current_task;
    }

    // Set the current task as the new task
    _current_task = next_task;

    // Check that there is a next task to be executed
    if (next_task != nullptr) {
        // Reset the quantum of the current task
        current_task->value().quantum = 0;

        // Reset the state of the current task if it's not blocked
        if (current_task->value().state == thread_state::running) {
            current_task->value().state = thread_state::ready;
        }

        // Set the new state of the new task
        next_task->value().state = thread_state::running;

        // Don't switch tasks if the new task is the current task
        if (next_task != current_task) {
            // Switch to the new task
            scheduler_utils::switch_task(&current_task->value(), &next_task->value(),
                                         &_processes[next_task->value().pid]);
        }
    } else {
        // TODO: Switch to idle thread
    }
}

influx::threading::tcb *influx::threading::scheduler::get_next_task() {
    process current_process = _processes[_current_task->value().pid];

    // Search for new task starting in the highest priority level
    for (int16_t i = MAX_PRIORITY_LEVEL; i >= 0; i--) {
        if (_priority_queues[i].next_task != nullptr) {
            return update_priority_queue_next_task(i);
        }
    }

    return nullptr;
}

influx::threading::tcb *influx::threading::scheduler::update_priority_queue_next_task(
    uint16_t priority) {
    kassert(priority >= 0 && priority <= MAX_PRIORITY_LEVEL);

    tcb *current_next_task = _priority_queues[priority].next_task;
    kassert(current_next_task != nullptr);

    tcb *new_next_task = current_next_task->next();

    // While we didn't reach the current next task
    while (new_next_task != current_next_task) {
        // If the thread is ready to be executed, set it as the next task
        if (new_next_task->value().state == thread_state::ready ||
            new_next_task->value().state == thread_state::running) {
            _priority_queues[priority].next_task = new_next_task;
            break;
        }
    }

    // If no new task was found, set the next task as null
    if (new_next_task == current_next_task) {
        _priority_queues[priority].next_task = nullptr;
    }

    return current_next_task;
}

void influx::threading::scheduler::tick_handler() {
    // If the current task reached the max quantum, reschedule
    if (_current_task->value().quantum == _max_quantum) {
        reschedule();
    } else {
        _current_task->value().quantum++;
    }
}