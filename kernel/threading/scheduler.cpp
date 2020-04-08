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
      _started(false),
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
                              .file_descriptors = structures::unique_hash_map<vfs::open_file>(),
                              .name = "kernel"});

    // Create kernel main thread
    _log("Creating kernel main thread..\n");
    _priority_queues[MAX_PRIORITY_LEVEL].start =
        new tcb(thread{.tid = _processes[0].threads.insert_unique(),
                       .pid = 0,
                       .context = nullptr,
                       .kernel_stack = (void *)get_stack_pointer(),
                       .state = thread_state::running,
                       .quantum = 0,
                       .sleep_quantum = 0});
    _priority_queues[MAX_PRIORITY_LEVEL].next_task = nullptr;
    _priority_queues[MAX_PRIORITY_LEVEL].start->prev() = _priority_queues[MAX_PRIORITY_LEVEL].start;
    _priority_queues[MAX_PRIORITY_LEVEL].start->next() = _priority_queues[MAX_PRIORITY_LEVEL].start;

    // Create kernel idle thread
    _log("Creating scheduler idle thread..\n");
    _idle_task = new tcb(thread({.tid = _processes[0].threads.insert_unique(),
                                 .pid = 0,
                                 .context = nullptr,
                                 .kernel_stack = memory::virtual_allocator::allocate(
                                     DEFAULT_KERNEL_STACK_SIZE, PROT_READ | PROT_WRITE),
                                 .state = thread_state::running,
                                 .quantum = 0,
                                 .sleep_quantum = 0}));
    _idle_task->value().context =
        (regs *)((uint8_t *)_idle_task->value().kernel_stack + DEFAULT_KERNEL_STACK_SIZE -
                 sizeof(regs) - sizeof(uint64_t));  // Set context pointer
    *(uint64_t *)((uint8_t *)_idle_task->value().kernel_stack + DEFAULT_KERNEL_STACK_SIZE -
                  sizeof(uint64_t)) = (uint64_t)
        utils::method_function_wrapper<scheduler, &scheduler::idle_task>;  // Set idle task function
    _idle_task->value().context->rdi =
        (uint64_t)this;  // Send this object to the idle task function

    // Set the current task as the kernel main thread
    _current_task = _priority_queues[MAX_PRIORITY_LEVEL].start;

    // Create a thread for tasks clean
    _log("Creating scheduler tasks clean thread..\n");
    _tasks_clean_task = create_kernel_thread(
        utils::method_function_wrapper<scheduler, &scheduler::tasks_clean_task>, this, true);

    // Register tick handler
    _log("Registering tick handler..\n");
    kernel::time_manager()->register_tick_handler(
        utils::method_function_wrapper<scheduler, &scheduler::tick_handler>, this);

    // Set the scheduler as started
    _started = true;
}

bool influx::threading::scheduler::started() const { return _started; }

influx::threading::tcb *influx::threading::scheduler::create_kernel_thread(void (*func)(),
                                                                           void *data,
                                                                           bool blocked) {
    void *stack =
        memory::virtual_allocator::allocate(DEFAULT_KERNEL_STACK_SIZE, PROT_READ | PROT_WRITE);

    regs *context =
        (regs *)((uint8_t *)stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(regs) - sizeof(uint64_t));

    interrupts_lock int_lk;
    tcb *new_task = new tcb(thread{.tid = _processes[0].threads.insert_unique(),
                                   .pid = 0,
                                   .context = context,
                                   .kernel_stack = stack,
                                   .state = blocked ? thread_state::blocked : thread_state::ready,
                                   .quantum = 0,
                                   .sleep_quantum = 0});
    int_lk.unlock();

    _log("Creating kernel thread with function %p and data %p..\n", func, data);

    // Set the RIP to return to when the thread is selected
    *(uint64_t *)((uint8_t *)stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(uint64_t)) =
        (uint64_t)new_thread_wrapper;

    // Send to the new thread wrapper the thread function and it's data
    context->rdi = (uint64_t)func;
    context->rsi = (uint64_t)data;

    // Queue the task
    queue_task(new_task);

    return new_task;
}

influx::threading::tcb *influx::threading::scheduler::create_kernel_thread(void (*func)(void *),
                                                                           void *data,
                                                                           bool blocked) {
    return create_kernel_thread((void (*)())func, data, blocked);
}

void influx::threading::scheduler::reschedule() {
    interrupts_lock int_lk;

    tcb *current_task = _current_task;
    tcb *next_task = get_next_task();

    // If there isn't a next task and the current task isn't blocked, keep running it
    if (next_task == nullptr && current_task->value().state == thread_state::running) {
        next_task = current_task;
    } else if (next_task == nullptr)  // If there isn't no ready task, jump to the idle task
    {
        next_task = _idle_task;
    }

    // Set the current task as the new task
    _current_task = next_task;

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

        // Move to the next node
        new_next_task = new_next_task->next();
    }

    // If no new task was found, set the next task as null
    if (new_next_task == current_next_task) {
        _priority_queues[priority].next_task = nullptr;
    }

    return current_next_task;
}

void influx::threading::scheduler::tick_handler() {
    // Update sleeping tasks
    update_tasks_sleep_quantum();

    // If the current task reached the max quantum, reschedule
    if (_current_task->value().quantum == _max_quantum) {
        reschedule();
    } else {
        _current_task->value().quantum++;
    }
}

void influx::threading::scheduler::update_tasks_sleep_quantum() {
    interrupts_lock int_lk;

    tcb *task = nullptr;

    // For each priority queue
    for (auto &priority_queue : _priority_queues) {
        task = priority_queue.start;
        if (task == nullptr) {
            continue;
        }

        // For each sleeping task update sleep quantum
        do {
            // Update sleep quantum
            if (task->value().state == thread_state::sleeping) {
                task->value().sleep_quantum -= (kernel::time_manager()->timer_frequency() / 1000);

                // Check if the task finished sleeping
                if (task->value().sleep_quantum == 0) {
                    task->value().state = thread_state::ready;

                    // If there is no next task, set the new ready task as the next task
                    if (priority_queue.next_task == nullptr) {
                        priority_queue.next_task = task;
                    }
                }
            }

            // Move to the next task
            task = task->next();
        } while (task != priority_queue.start);
    }
}

void influx::threading::scheduler::sleep(uint64_t ms) {
    kassert(ms != 0);

    // Set the task's sleep quantum
    _current_task->value().sleep_quantum = ms * (kernel::time_manager()->timer_frequency() / 1000);

    // Set the task's state to sleeping
    _current_task->value().state = thread_state::sleeping;

    // Re-schedule to another task
    reschedule();
}

void influx::threading::scheduler::kill_current_task() {
    interrupts_lock int_lk;

    process &current_process = _processes[_current_task->value().pid];
    priority_tcb_queue &task_priority_queue = _priority_queues[current_process.priority];

    // If the current task is the first task in the priority queue, set the start as the next task
    if (task_priority_queue.start == _current_task && _current_task->next() != _current_task) {
        task_priority_queue.start = _current_task->next();
    } else if (task_priority_queue.start == _current_task) {
        task_priority_queue.start = nullptr;
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

    // Unblock the tasks clean task
    unblock_task(_tasks_clean_task);

    // Re-schedule to another task
    reschedule();
}

void influx::threading::scheduler::block_task(influx::threading::tcb *task) {
    interrupts_lock int_lk;

    process &task_process = _processes[task->value().pid];
    priority_tcb_queue &task_priority_queue = _priority_queues[task_process.priority];

    // If the task isn't already blocked
    if (task->value().state != thread_state::blocked) {
        if (task_priority_queue.next_task == task) {
            // Remove the task as the next task and update the priority queue
            update_priority_queue_next_task(task_process.priority);
        }

        // Set the task state as blocked
        task->value().state = thread_state::blocked;
    }
}

void influx::threading::scheduler::block_current_task() {
    // Block this task and reschedule to another task
    block_task(_current_task);
    reschedule();
}

void influx::threading::scheduler::unblock_task(influx::threading::tcb *task) {
    interrupts_lock int_lk;

    process &task_process = _processes[task->value().pid];
    priority_tcb_queue &task_priority_queue = _priority_queues[task_process.priority];

    // If the task is blocked
    if (task->value().state == thread_state::blocked) {
        // Update the state of the task to ready
        task->value().state = thread_state::ready;

        // Check if the task's priority queue doesn't have a next task
        if (task_priority_queue.next_task == nullptr) {
            task_priority_queue.next_task = task;
        }
    }
}

influx::threading::tcb *influx::threading::scheduler::get_current_task() const {
    return _current_task;
}

uint64_t influx::threading::scheduler::get_current_task_id() const {
    return _current_task->value().tid;
}

uint64_t influx::threading::scheduler::get_current_process_id() const {
    return _current_task->value().pid;
}

uint64_t influx::threading::scheduler::add_file_descriptor(const influx::vfs::open_file &file) {
    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];

    return process.file_descriptors.insert_unique(file);
}

influx::vfs::error influx::threading::scheduler::get_file_descriptor(uint64_t fd,
                                                                     influx::vfs::open_file &file) {
    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];

    // If the file descriptor isn't found return error
    if (process.file_descriptors.count(fd) == 0) {
        return vfs::error::invalid_file;
    }

    // Get the file
    file = process.file_descriptors[fd];

    return vfs::error::success;
}

void influx::threading::scheduler::update_file_descriptor(uint64_t fd,
                                                          influx::vfs::open_file &file) {
    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];

    process.file_descriptors[fd] = file;
}

void influx::threading::scheduler::remove_file_descriptor(uint64_t fd) {
    process &process = _processes[_current_task->value().pid];

    process.file_descriptors.erase(fd);
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
            delete[](uint64_t *) task->value().kernel_stack;

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
                int_lk.lock();
                _processes[task_process.pid] = task_process;
                int_lk.unlock();
            }
        }

        // Block this task
        block_current_task();
    }
}

void influx::threading::scheduler::idle_task() {
    // Enable interrupts
    kernel::interrupt_manager()->enable_interrupts();

    while (true) {
        __asm__ __volatile__("hlt");  // Wait for interrupt

        // Try to reschedule to another task since probably some task is available
        reschedule();
    }
}

void influx::threading::scheduler::queue_task(influx::threading::tcb *task) {
    interrupts_lock int_lk;

    process &new_task_process = _processes[task->value().pid];
    priority_tcb_queue &new_task_priority_queue = _priority_queues[new_task_process.priority];

    _log("Queuing task %p..\n", task);

    // If the queue linked list is empty
    if (new_task_priority_queue.start == nullptr) {
        new_task_priority_queue.start = task;
        task->prev() = task;
        task->next() = task;
    } else {
        // Insert the task into the priority queue linked list
        new_task_priority_queue.start->prev()->next() = task;
        task->prev() = new_task_priority_queue.start->prev();
        task->next() = new_task_priority_queue.start;
        new_task_priority_queue.start->prev() = task;
    }

    // Check if there is no next task and the new task is ready
    if (new_task_priority_queue.next_task == nullptr &&
        task->value().state == thread_state::ready) {
        new_task_priority_queue.next_task = task;
    }
}

uint64_t influx::threading::scheduler::get_stack_pointer() const {
    uint64_t rsp;

    __asm__ __volatile__("mov %0, rsp" : "=r"(rsp));

    return rsp;
}