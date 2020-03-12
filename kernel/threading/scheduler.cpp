#include <kernel/threading/scheduler.h>

#include <kernel/algorithm.h>
#include <kernel/assert.h>
#include <kernel/kernel.h>
#include <kernel/memory/virtual_allocator.h>
#include <kernel/threading/interrupts_lock.h>
#include <kernel/threading/scheduler_utils.h>
#include <kernel/time/time_manager.h>
#include <kernel/utils.h>
#include <memory/protection_flags.h>

void influx::threading::new_thread_wrapper(void (*func)(void *), void *data) {
    // Re-enable interrupts since they were disabled in the reschedule function
    kernel::interrupt_manager()->enable_interrupts();

    // Call the thread function
    func(data);

    // Kill the task
    kernel::scheduler()->kill_current_task();
}

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
                              .threads = structures::unique_vector(),
                              .name = "kernel"});

    // Create kernel main thread
    _log("Creating kernel main thread..\n");
    _priority_queues[MAX_PRIORITY_LEVEL].start =
        new tcb(thread{.tid = _processes[0].threads.insert_unique(),
                       .pid = 0,
                       .context = nullptr,
                       .kernel_stack = (void *)get_stack_pointer(),
                       .state = thread_state::running,
                       .quantum = 0});
    _priority_queues[MAX_PRIORITY_LEVEL].next_task = nullptr;
    _priority_queues[MAX_PRIORITY_LEVEL].start->prev() = _priority_queues[MAX_PRIORITY_LEVEL].start;
    _priority_queues[MAX_PRIORITY_LEVEL].start->next() = _priority_queues[MAX_PRIORITY_LEVEL].start;

    // Set the current task as the kernel main thread
    _current_task = _priority_queues[MAX_PRIORITY_LEVEL].start;

    // Create a thread for tasks clean
    _log("Creating thread for tasks clean..\n");
    create_kernel_thread(utils::method_function_wrapper<scheduler, &scheduler::tasks_clean_task>, this);

    // Register tick handler
    _log("Registering tick handler..\n");
    kernel::time_manager()->register_tick_handler(
        utils::method_function_wrapper<scheduler, &scheduler::tick_handler>, this);
}

const influx::threading::thread &influx::threading::scheduler::create_kernel_thread(void (*func)(),
                                                                                    void *data) {
    void *stack =
        memory::virtual_allocator::allocate(DEFAULT_KERNEL_STACK_SIZE, PROT_READ | PROT_WRITE);

    regs *context = (regs *)((uint8_t *)stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(regs) - sizeof(uint64_t));

    tcb *new_task = new tcb(thread{.tid = _processes[0].threads.insert_unique(),
                                   .pid = 0,
                                   .context = context,
                                   .kernel_stack = stack,
                                   .state = thread_state::ready,
                                   .quantum = 0});

    _log("Creating kernel thread with function %p and data %p..\n", func, data);

    // Set the RIP to return to when the thread is selected
    *(uint64_t *)((uint8_t *)stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(uint64_t)) = (uint64_t)new_thread_wrapper;

    // Send to the new thread wrapper the thread function and it's data
    context->rdi = (uint64_t)func;
    context->rsi = (uint64_t)data;

    // Queue the task
    queue_task(new_task);

    return new_task->value();
}

const influx::threading::thread &influx::threading::scheduler::create_kernel_thread(
    void (*func)(void *), void *data) {
    return create_kernel_thread((void (*)())func, data);
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

void influx::threading::scheduler::kill_current_task() {
    interrupts_lock int_lk;

    process &current_process = _processes[_current_task->value().pid];
    priority_tcb_queue &task_priority_queue = _priority_queues[current_process.priority];

    // If the current task is the first task in the priority queue, set the start as the next task
    if (task_priority_queue.start == _current_task) {
        task_priority_queue.start = _current_task->next();
    }

    // Remove the task from the priority queue
    _current_task->prev()->next() = _current_task->next();
    _current_task->next()->prev() = _current_task->prev();

    // Add the task to the killed tasks queue
    _killed_tasks_queue.push_back(_current_task);

    // Set the task as killed
    _current_task->value().state = thread_state::killed;

    // Unlock the interrupts lock
    int_lk.unlock();

    // Re-schedule to the another task
    reschedule();
}

void influx::threading::scheduler::tasks_clean_task() {
    interrupts_lock int_lk(false);

    while (true) {
        structures::vector<tcb *> killed_tasks_queue;

        // Create a copy of the current killed tasks queue
        int_lk.lock();
        killed_tasks_queue = _killed_tasks_queue;
        _killed_tasks_queue.clear();
        int_lk.unlock();

        // For each task to be killed
        for (tcb *&task : killed_tasks_queue) {
            // Create a copy of the process struct of the task
            int_lk.lock();
            process task_process = _processes[task->value().pid];
            int_lk.unlock();

            // If the process is a user process
            if (!task_process.system) {
                // TODO: Handle user tasks kill
            }

            // Remove the thread from the threads list of the process
            task_process.threads.erase(algorithm::find(
                task_process.threads.begin(), task_process.threads.end(), task->value().tid));

            // Free the task kernel stack
            delete[] (uint64_t *)task->value().kernel_stack;

            // Free the task object
            delete task;

            // If the process has no threads left, kill the process
            if (task_process.threads.empty()) {
                if (!task_process.system) {
                    // TODO: Handle user process kill
                }

                // Delete the process object
                int_lk.lock();
                _processes.erase(task_process.pid);
                int_lk.unlock();
            } else {
                // Update the process object
                _processes[task_process.pid] = task_process;
            }
        }

        // Reschedule the task
        reschedule();
    }
}

void influx::threading::scheduler::queue_task(influx::threading::tcb *task) {
    interrupts_lock int_lk;

    process &new_task_process = _processes[task->value().pid];
    priority_tcb_queue &new_task_priority_queue = _priority_queues[new_task_process.priority];

    _log("Queuing task %p..\n", task);

    // Insert the task into the priority queue linked list
    new_task_priority_queue.start->prev()->next() = task;
    task->prev() = new_task_priority_queue.start->prev();
    task->next() = new_task_priority_queue.start;
    new_task_priority_queue.start->prev() = task;

    // Check if there is no next task
    if (new_task_priority_queue.next_task == nullptr) {
        new_task_priority_queue.next_task = task;
    }
}

uint64_t influx::threading::scheduler::get_stack_pointer() const {
    uint64_t rsp;

    __asm__ __volatile__("mov %0, rsp" : "=r"(rsp));

    return rsp;
}