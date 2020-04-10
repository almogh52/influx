#include <kernel/threading/scheduler.h>

#include <kernel/algorithm.h>
#include <kernel/assert.h>
#include <kernel/kernel.h>
#include <kernel/memory/paging_manager.h>
#include <kernel/memory/virtual_allocator.h>
#include <kernel/threading/interrupts_lock.h>
#include <kernel/threading/scheduler_utils.h>
#include <kernel/time/time_manager.h>
#include <kernel/utils.h>
#include <memory/protection_flags.h>

void influx::threading::new_kernel_thread_wrapper(void (*func)(void *), void *data) {
    // Re-enable interrupts since they were disabled in the reschedule function
    kernel::interrupt_manager()->enable_interrupts();

    // Call the thread function
    func(data);

    // Kill the task
    kernel::scheduler()->kill_current_task();
}

void influx::threading::new_user_process_wrapper(influx::elf_file &exec_file) {
    uint64_t user_stack_virtual_address =
        (uint64_t)kernel::scheduler()->_current_task->value().user_stack;

    // Re-enable interrupts since they were disabled in the reschedule function
    kernel::interrupt_manager()->enable_interrupts();

    // Load each segment to the user's memory space
    for (const auto &segment : exec_file.segments()) {
        // Check valid segment location
        if (segment.virtual_address < HIGHER_HALF_KERNEL_OFFSET &&
            segment.virtual_address + segment.data.size() < HIGHER_HALF_KERNEL_OFFSET) {
            // For each page of the segment map it
            for (uint64_t page_addr = segment.virtual_address;
                 page_addr < (segment.virtual_address + segment.data.size());
                 page_addr += PAGE_SIZE) {
                if (!memory::paging_manager::map_page(page_addr)) {
                    kernel::scheduler()->kill_current_task();
                }

                // Set map permissions
                memory::paging_manager::set_pte_permissions(page_addr, segment.protection, true);
            }

            // Copy segment data to the memory
            memory::utils::memcpy((void *)segment.virtual_address, segment.data.data(),
                                  segment.data.size());
        } else {
            kernel::scheduler()->kill_current_task();
        }
    }

    // Map user stack
    for (uint64_t stack_offset = 0; stack_offset < DEFAULT_USER_STACK_SIZE;
         stack_offset += PAGE_SIZE) {
        if (!memory::paging_manager::map_page(DEFAULT_USER_STACK_ADDRESS + stack_offset,
                                              memory::paging_manager::get_physical_address(
                                                  user_stack_virtual_address + stack_offset))) {
            kernel::scheduler()->kill_current_task();
        }

        // Set R/W permission and DPL of ring 3
        memory::paging_manager::set_pte_permissions(DEFAULT_USER_STACK_ADDRESS + stack_offset,
                                                    PROT_READ | PROT_WRITE, true);
    }

    // Jump to the process entry point
    scheduler_utils::jump_to_ring_3(
        exec_file.entry_address(),
        (void *)(DEFAULT_USER_STACK_ADDRESS + DEFAULT_USER_STACK_SIZE - 8), (void *)0x0);
}

influx::threading::scheduler::scheduler(uint64_t tss_addr)
    : _log("Scheduler", console_color::blue),
      _started(false),
      _priority_queues(MAX_PRIORITY_LEVEL + 1),
      _current_task(nullptr),
      _max_quantum((kernel::time_manager()->timer_frequency() / 1000) * TASK_MAX_TIME_SLICE),
      _tss((tss_t *)tss_addr) {
    kassert(_max_quantum != 0);

    // Create kernel process
    _log("Creating kernel process..\n");
    _processes.insert_unique({.pid = 0,
                              .ppid = 0,
                              .priority = MAX_PRIORITY_LEVEL,
                              .system = true,
                              .cr3 = (uint64_t)memory::utils::get_pml4(),
                              .pml4t = nullptr,
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
                       .user_stack = nullptr,
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
                                 .user_stack = nullptr,
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
                                   .user_stack = nullptr,
                                   .state = blocked ? thread_state::blocked : thread_state::ready,
                                   .quantum = 0,
                                   .sleep_quantum = 0});
    int_lk.unlock();

    _log("Creating kernel thread with function %p and data %p..\n", func, data);

    // Set the RIP to return to when the thread is selected
    *(uint64_t *)((uint8_t *)stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(uint64_t)) =
        (uint64_t)new_kernel_thread_wrapper;

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
        // Set TSS kernel stack pointer for userspace programs
        if (!_processes[next_task->value().pid].system) {
            _tss->rsp0_low = (uint64_t)next_task->value().context & 0xFFFFFFFF;
            _tss->rsp0_high = ((uint64_t)next_task->value().context >> 32) & 0xFFFFFFFF;
        }

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
    if (_priority_queues[priority].next_task->value().state != thread_state::ready &&
        _priority_queues[priority].next_task->value().state != thread_state::running) {
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

    // If it is the next task, set the next task as null
    if (task_priority_queue.next_task == _current_task) {
        task_priority_queue.next_task = nullptr;
    }

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

uint64_t influx::threading::scheduler::exec(influx::elf_file &exec_file,
                                            const influx::structures::string exec_name) {
    uint64_t pid = 0;
    pml4e_t *pml4t = 0;

    void *kernel_stack = nullptr;
    void *user_stack = nullptr;
    regs *context = nullptr;
    tcb *task = nullptr;

    // If the file wasn't parsed
    if (!exec_file.parsed()) {
        return 0;
    }

    // Allocate PML4T
    pml4t = (pml4e_t *)memory::virtual_allocator::allocate(PAGE_SIZE, PROT_READ | PROT_WRITE);
    if (pml4t == 0) {
        return 0;
    }

    // Initiate PML4T for the userland executable
    memory::paging_manager::init_user_process_paging((uint64_t)pml4t);

    // Create the process for the executable
    pid = _processes.insert_unique(process());
    _processes[pid] = {.pid = pid,
                       .ppid = _current_task->value().pid,
                       .priority = DEFAULT_USER_SPACE_PROCESS_PRIORITY,
                       .system = false,
                       .cr3 = memory::paging_manager::get_physical_address((uint64_t)pml4t),
                       .pml4t = pml4t,
                       .threads = structures::unique_vector(),
                       .file_descriptors = structures::unique_hash_map<vfs::open_file>(),
                       .name = exec_name};

    // Allocate kernel stack for main process task
    kernel_stack =
        memory::virtual_allocator::allocate(DEFAULT_KERNEL_STACK_SIZE, PROT_READ | PROT_WRITE);
    if (!kernel_stack) {
        _processes.erase(pid);
        return 0;
    }
    context = (regs *)((uint8_t *)kernel_stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(regs) -
                       sizeof(uint64_t));

    // Allocate user stack for main process task
    user_stack =
        memory::virtual_allocator::allocate(DEFAULT_USER_STACK_SIZE, PROT_READ | PROT_WRITE);
    if (!user_stack) {
        memory::virtual_allocator::free(user_stack, DEFAULT_KERNEL_STACK_SIZE);
        _processes.erase(pid);
        return 0;
    }

    // Create main task for the process
    task = new tcb(thread{.tid = _processes[pid].threads.insert_unique(),
                          .pid = pid,
                          .context = context,
                          .kernel_stack = kernel_stack,
                          .user_stack = user_stack,
                          .state = thread_state::ready,
                          .quantum = 0,
                          .sleep_quantum = 0});

    // Set the RIP to return to when the thread is selected
    *(uint64_t *)((uint8_t *)kernel_stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(uint64_t)) =
        (uint64_t)new_user_process_wrapper;

    // Send to the new thread wrapper the thread function and it's data
    context->rdi = (uint64_t)&exec_file;

    // Queue the task
    queue_task(task);

    return pid;
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