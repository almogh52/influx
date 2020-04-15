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

void influx::threading::new_user_process_wrapper(influx::threading::executable *exec) {
    // Re-enable interrupts since they were disabled in the reschedule function
    kernel::interrupt_manager()->enable_interrupts();

    uint64_t user_stack_virtual_address =
        (uint64_t)kernel::scheduler()->_current_task->value().user_stack;

    uint64_t argv_envp_pages = kernel::scheduler()->pages_for_argv_envp(*exec);
    structures::pair<const char **, const char **> function_ptrs;

    uint64_t argc = exec->args.size(), entry = exec->file.entry_address();

    uint64_t end_of_executable = 0;

    // Get the process object
    interrupts_lock int_lk;
    process &process =
        kernel::scheduler()->_processes[kernel::scheduler()->_current_task->value().pid];
    int_lk.unlock();

    // Load each segment to the user's memory space
    for (const auto &seg : exec->file.segments()) {
        // Check valid segment location
        if (seg.virtual_address < USERLAND_MEMORY_BARRIER &&
            seg.virtual_address + seg.data.size() <= USERLAND_MEMORY_BARRIER) {
            // For each page of the segment map it
            for (uint64_t page_addr = seg.virtual_address;
                 page_addr < (seg.virtual_address + seg.data.size()); page_addr += PAGE_SIZE) {
                if (!memory::paging_manager::map_page(page_addr)) {
                    delete exec;
                    kernel::scheduler()->kill_current_task();
                }

                // Set map permissions
                memory::paging_manager::set_pte_permissions(page_addr, seg.protection, true);
            }

            // Copy segment data to the memory
            memory::utils::memcpy((void *)seg.virtual_address, seg.data.data(), seg.data.size());

            // Add segment to vector of segments
            process.segments += segment{.virtual_address = seg.virtual_address,
                                        .size = seg.data.size(),
                                        .protection = seg.protection};

            // Update end of executable
            if (seg.virtual_address + seg.data.size() > end_of_executable) {
                end_of_executable = seg.virtual_address + seg.data.size();
            }
        } else {
            delete exec;
            kernel::scheduler()->kill_current_task();
        }
    }

    // Allocate pages for arguments and environment variables
    for (uint64_t page = 0; page < argv_envp_pages; page++) {
        if (!memory::paging_manager::map_page(USERLAND_MEMORY_BARRIER - (page + 1) * PAGE_SIZE)) {
            delete exec;
            kernel::scheduler()->kill_current_task();
        }

        // Set R/W permission and DPL of ring 0
        memory::paging_manager::set_pte_permissions(
            USERLAND_MEMORY_BARRIER - (page + 1) * PAGE_SIZE, PROT_READ | PROT_WRITE, false);
    }

    // Copy args and env variables
    function_ptrs = kernel::scheduler()->copy_argv_envp(
        *exec, USERLAND_MEMORY_BARRIER - (argv_envp_pages * PAGE_SIZE));

    // Set permissions for arguments and environment variables pages
    for (uint64_t page = 0; page < argv_envp_pages; page++) {
        // Set read permission and DPL of ring 3
        memory::paging_manager::set_pte_permissions(
            USERLAND_MEMORY_BARRIER - (page + 1) * PAGE_SIZE, PROT_READ, true);
    }

    // Map user stack
    for (uint64_t stack_offset = 0; stack_offset < DEFAULT_USER_STACK_SIZE;
         stack_offset += PAGE_SIZE) {
        if (!memory::paging_manager::map_page(
                DEFAULT_USER_STACK_ADDRESS + stack_offset - (argv_envp_pages * PAGE_SIZE),
                memory::paging_manager::get_physical_address(user_stack_virtual_address +
                                                             stack_offset) /
                    PAGE_SIZE)) {
            delete exec;
            kernel::scheduler()->kill_current_task();
        }

        // Set R/W permission and DPL of ring 3
        memory::paging_manager::set_pte_permissions(
            DEFAULT_USER_STACK_ADDRESS + stack_offset - (argv_envp_pages * PAGE_SIZE),
            PROT_READ | PROT_WRITE, true);
    }

    // Align the end of the exeuctable
    end_of_executable +=
        end_of_executable % PAGE_SIZE ? (PAGE_SIZE - (end_of_executable % PAGE_SIZE)) : 0;

    // Set program break
    process.program_break_start = end_of_executable;
    process.program_break_end = end_of_executable;

    // Set args size
    int_lk.lock();
    kernel::scheduler()->_current_task->value().args_size = argv_envp_pages * PAGE_SIZE;
    int_lk.unlock();

    // Free executable object
    delete exec;

    // Jump to the process entry point
    scheduler_utils::jump_to_ring_3(entry,
                                    (void *)(DEFAULT_USER_STACK_ADDRESS + DEFAULT_USER_STACK_SIZE -
                                             8 - (argv_envp_pages * PAGE_SIZE)),
                                    argc, function_ptrs.first, function_ptrs.second);
}

void influx::threading::new_fork_process_wrapper(
    influx::structures::vector<influx::file_segment> *segments,
    influx::interrupts::regs *old_context) {
    // Re-enable interrupts since they were disabled in the reschedule function
    kernel::interrupt_manager()->enable_interrupts();

    // Create a stack copy of the old context
    interrupts::regs old_context_var = *old_context;

    // Delete the old context object
    delete old_context;

    // Allocate and map the different sections of the executable in the memory
    for (const auto &seg : *segments) {
        // Check valid segment location
        if (seg.virtual_address < USERLAND_MEMORY_BARRIER &&
            seg.virtual_address + seg.data.size() <= USERLAND_MEMORY_BARRIER) {
            // For each page of the segment map it
            for (uint64_t page_addr = seg.virtual_address;
                 page_addr < (seg.virtual_address + seg.data.size()); page_addr += PAGE_SIZE) {
                if (!memory::paging_manager::map_page(page_addr)) {
                    delete segments;
                    kernel::scheduler()->kill_current_task();
                }

                // Set map permissions
                memory::paging_manager::set_pte_permissions(page_addr, seg.protection, true);
            }

            // Copy segment data to the memory
            memory::utils::memcpy((void *)seg.virtual_address, seg.data.data(), seg.data.size());
        } else {
            delete segments;
            kernel::scheduler()->kill_current_task();
        }
    }

    // Free segments vector and old context
    delete segments;

    // Return to the new process
    scheduler_utils::return_to_fork_process(old_context_var);
}

influx::threading::scheduler::scheduler(uint64_t tss_addr)
    : _log("Scheduler", console_color::blue),
      _started(false),
      _priority_queues(MAX_PRIORITY_LEVEL + 1),
      _current_task(nullptr),
      _max_quantum((kernel::time_manager()->timer_frequency() / 1000) * TASK_MAX_TIME_SLICE),
      _tss((tss_t *)tss_addr),
      _init_process(this) {
    kassert(_max_quantum != 0);

    // Create kernel process
    _log("Creating kernel process..\n");
    _processes.insert_unique({.pid = KERNEL_PID,
                              .ppid = KERNEL_PID,
                              .priority = MAX_PRIORITY_LEVEL,
                              .system = true,
                              .cr3 = (uint64_t)memory::utils::get_pml4(),
                              .pml4t = nullptr,
                              .program_break_start = 0,
                              .program_break_end = 0,
                              .threads = structures::unique_vector(),
                              .child_processes = structures::vector<uint64_t>(),
                              .file_descriptors = structures::unique_hash_map<vfs::open_file>(),
                              .name = "kernel",
                              .segments = structures::vector<segment>(),
                              .terminated = false});

    // Create kernel main thread
    _log("Creating kernel main thread..\n");
    _priority_queues[MAX_PRIORITY_LEVEL].start =
        new tcb(thread{.tid = _processes[0].threads.insert_unique(),
                       .pid = KERNEL_PID,
                       .context = nullptr,
                       .kernel_stack = (void *)get_stack_pointer(),
                       .user_stack = nullptr,
                       .args_size = 0,
                       .state = thread_state::running,
                       .quantum = 0,
                       .sleep_quantum = 0,
                       .child_wait_pid = 0});
    _priority_queues[MAX_PRIORITY_LEVEL].next_task = nullptr;
    _priority_queues[MAX_PRIORITY_LEVEL].start->prev() = _priority_queues[MAX_PRIORITY_LEVEL].start;
    _priority_queues[MAX_PRIORITY_LEVEL].start->next() = _priority_queues[MAX_PRIORITY_LEVEL].start;

    // Create kernel idle thread
    _log("Creating scheduler idle thread..\n");
    _idle_task = new tcb(thread{.tid = _processes[KERNEL_PID].threads.insert_unique(),
                                .pid = KERNEL_PID,
                                .context = nullptr,
                                .kernel_stack = memory::virtual_allocator::allocate(
                                    DEFAULT_KERNEL_STACK_SIZE, PROT_READ | PROT_WRITE),
                                .user_stack = nullptr,
                                .args_size = 0,
                                .state = thread_state::ready,
                                .quantum = 0,
                                .sleep_quantum = 0,
                                .child_wait_pid = 0});
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

    // Create init process
    _log("Creating init process..\n");
    _processes[KERNEL_PID].child_processes += INIT_PROCESS_PID;
    _processes.insert_unique({.pid = INIT_PROCESS_PID,
                              .ppid = KERNEL_PID,
                              .priority = MAX_PRIORITY_LEVEL,
                              .system = true,
                              .cr3 = (uint64_t)memory::utils::get_pml4(),
                              .pml4t = nullptr,
                              .program_break_start = 0,
                              .program_break_end = 0,
                              .threads = structures::unique_vector(),
                              .child_processes = structures::vector<uint64_t>(),
                              .file_descriptors = structures::unique_hash_map<vfs::open_file>(),
                              .name = "init",
                              .segments = structures::vector<segment>(),
                              .terminated = false});

    // Start init process
    _log("Starting init process..\n");
    _init_process.start();

    // Register tick handler
    _log("Registering tick handler..\n");
    kernel::time_manager()->register_tick_handler(
        utils::method_function_wrapper<scheduler, &scheduler::tick_handler>, this);

    // Set the scheduler as started
    _started = true;
}

bool influx::threading::scheduler::started() const { return _started; }

influx::threading::tcb *influx::threading::scheduler::create_kernel_thread(void (*func)(),
                                                                           void *data, bool blocked,
                                                                           uint64_t pid) {
    interrupts_lock int_lk;
    kassert(_processes.count(pid) != 0 && _processes[pid].system);

    void *stack =
        memory::virtual_allocator::allocate(DEFAULT_KERNEL_STACK_SIZE, PROT_READ | PROT_WRITE);

    regs *context =
        (regs *)((uint8_t *)stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(regs) - sizeof(uint64_t));

    tcb *new_task = new tcb(thread{.tid = _processes[pid].threads.insert_unique(),
                                   .pid = pid,
                                   .context = context,
                                   .kernel_stack = stack,
                                   .user_stack = nullptr,
                                   .args_size = 0,
                                   .state = blocked ? thread_state::blocked : thread_state::ready,
                                   .quantum = 0,
                                   .sleep_quantum = 0,
                                   .child_wait_pid = 0});
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
                                                                           void *data, bool blocked,
                                                                           uint64_t pid) {
    return create_kernel_thread((void (*)())func, data, blocked, pid);
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

    interrupts_lock int_lk;

    process &current_process = _processes[_current_task->value().pid];
    priority_tcb_queue &task_priority_queue = _priority_queues[current_process.priority];

    // Set the task's sleep quantum
    _current_task->value().sleep_quantum = ms * (kernel::time_manager()->timer_frequency() / 1000);

    // Set the task's state to sleeping
    _current_task->value().state = thread_state::sleeping;

    // If it is the next task, set the next task as null
    if (task_priority_queue.next_task == _current_task) {
        task_priority_queue.next_task = nullptr;
    }
    int_lk.unlock();

    // Re-schedule to another task
    reschedule();
}

int64_t influx::threading::scheduler::wait_for_child(int64_t child_pid) {
    // ** -1 for all children **

    // Verify child pid
    if (child_pid < WAIT_FOR_ANY_PROCESS || child_pid == 0) {
        return -1;
    }

    interrupts_lock int_lk;

    process &current_process = _processes[_current_task->value().pid];
    priority_tcb_queue &task_priority_queue = _priority_queues[current_process.priority];

    // Verify that the current process has child processes and that the target process exists and is
    // a child of the current process
    if ((child_pid == WAIT_FOR_ANY_PROCESS && current_process.child_processes.empty()) ||
        (child_pid != WAIT_FOR_ANY_PROCESS &&
         (_processes.count(child_pid) == 0 ||
          _processes[child_pid].ppid != _current_task->value().pid))) {
        return -1;
    }

    // If the child had already terminated
    if (child_pid != WAIT_FOR_ANY_PROCESS && _processes[child_pid].terminated) {
        // Remove the process object
        _processes.erase(child_pid);

        // Remove the process as a child prcess
        current_process.child_processes.erase(
            algorithm::find(current_process.child_processes.begin(),
                            current_process.child_processes.end(), (uint64_t)child_pid));

        return child_pid;
    } else if (child_pid == WAIT_FOR_ANY_PROCESS) {
        // Search for a terminated process
        for (auto &pid : current_process.child_processes) {
            if (_processes[pid].terminated) {
                // Remove the process object
                _processes.erase(pid);

                // Remove the process as a child prcess
                current_process.child_processes.erase(&pid);

                return pid;
            }
        }
    }

    // Set the child pid that the task is waiting for
    _current_task->value().child_wait_pid = child_pid;

    // Set the task's state to waiting
    _current_task->value().state = thread_state::waiting_for_child;

    // If it is the next task, set the next task as null
    if (task_priority_queue.next_task == _current_task) {
        task_priority_queue.next_task = nullptr;
    }
    int_lk.unlock();

    // Re-schedule to another task
    reschedule();

    return _current_task->value().child_wait_pid;
}

void influx::threading::scheduler::kill_current_task() {
    interrupts_lock int_lk;

    process &current_process = _processes[_current_task->value().pid];
    priority_tcb_queue &task_priority_queue = _priority_queues[current_process.priority];

    // If it's the last thread of the user process, free it's memory
    if (!current_process.system && current_process.threads.size() == 1) {
        int_lk.unlock();
        memory::paging_manager::free_user_process_paging();
        int_lk.lock();
    }

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

    // If it is the next task, set the next task as null
    if (task_priority_queue.next_task == _current_task) {
        task_priority_queue.next_task = nullptr;
    }

    // Set the task as killed
    _current_task->value().state = thread_state::killed;

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
        // Set the task state as blocked
        task->value().state = thread_state::blocked;

        if (task_priority_queue.next_task == task) {
            // Remove the task as the next task and update the priority queue
            update_priority_queue_next_task(task_process.priority);
        }
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
    if (task->value().state == thread_state::blocked ||
        task->value().state == thread_state::waiting_for_child) {
        // Update the state of the task to ready
        task->value().state = thread_state::ready;

        // Check if the task's priority queue doesn't have a next task
        if (task_priority_queue.next_task == nullptr ||
            task_priority_queue.next_task == _current_task) {
            task_priority_queue.next_task = task;
        }
    }
}

uint64_t influx::threading::scheduler::exec(
    size_t fd, const influx::structures::string &name,
    const influx::structures::vector<influx::structures::string> &args,
    const influx::structures::vector<influx::structures::string> &env) {
    executable exec{.name = name, .file = elf_file(fd), .args = args, .env = env};

    // Try to parse the file
    if (!exec.file.parse()) {
        return 0;
    }

    // If the kernel is requesting to start a process, queue it in the init process
    if (_current_task->value().pid == KERNEL_PID) {
        _init_process.queue_exec(exec);
        return INIT_PROCESS_PID;
    } else {
        // Start the process
        return start_process(exec);
    }
}

uint64_t influx::threading::scheduler::fork(influx::interrupts::regs old_context) {
    file_segment seg;
    structures::vector<file_segment> segments, *segments_obj = nullptr;

    uint64_t pid = 0;
    pml4e_t *pml4t = 0;

    void *kernel_stack = nullptr;
    void *user_stack = nullptr;
    regs *context = nullptr;
    tcb *task = nullptr;

    interrupts::regs *old_context_obj = nullptr;

    interrupts_lock int_lk;
    process process_fork = _processes[_current_task->value().pid];
    int_lk.unlock();

    // Allocate PML4T
    pml4t = (pml4e_t *)memory::virtual_allocator::allocate(PAGE_SIZE, PROT_READ | PROT_WRITE);
    if (pml4t == 0) {
        return 0;
    }

    // Initiate PML4T for the userland executable
    memory::paging_manager::init_user_process_paging((uint64_t)pml4t);

    // Create a copy of all the segments of the current process
    for (const auto &exec_seg : process_fork.segments) {
        seg = file_segment{.virtual_address = exec_seg.virtual_address,
                           .data = structures::dynamic_buffer(exec_seg.size),
                           .protection = exec_seg.protection};

        // Copy the segment data
        memory::utils::memcpy(seg.data.data(), (void *)exec_seg.virtual_address, exec_seg.size);

        // Add the segment to the segments vector
        segments += seg;
    }

    // Create a copy of the program break
    seg = file_segment{.virtual_address = process_fork.program_break_start,
                       .data = structures::dynamic_buffer(process_fork.program_break_end -
                                                          process_fork.program_break_start),
                       .protection = PROT_READ | PROT_WRITE};
    memory::utils::memcpy(seg.data.data(), (void *)process_fork.program_break_start,
                          process_fork.program_break_end - process_fork.program_break_start);
    segments += seg;

    // Create a copy of the user stack
    seg = file_segment{
        .virtual_address = DEFAULT_USER_STACK_ADDRESS - _current_task->value().args_size,
        .data = structures::dynamic_buffer(DEFAULT_USER_STACK_SIZE),
        .protection = PROT_READ | PROT_WRITE};
    memory::utils::memcpy(seg.data.data(),
                          (void *)(DEFAULT_USER_STACK_ADDRESS - _current_task->value().args_size),
                          DEFAULT_USER_STACK_SIZE);
    segments += seg;

    // Create a copy of the args
    seg =
        file_segment{.virtual_address = USERLAND_MEMORY_BARRIER - _current_task->value().args_size,
                     .data = structures::dynamic_buffer(_current_task->value().args_size),
                     .protection = PROT_READ};
    memory::utils::memcpy(seg.data.data(),
                          (void *)(USERLAND_MEMORY_BARRIER - _current_task->value().args_size),
                          _current_task->value().args_size);
    segments += seg;

    // Fork the file descriptors
    kernel::vfs()->fork_file_descriptors(process_fork.file_descriptors);

    // Reset properties for the new process
    process_fork.cr3 = memory::paging_manager::get_physical_address((uint64_t)pml4t);
    process_fork.pml4t = pml4t;
    process_fork.threads = structures::unique_vector();
    process_fork.child_processes = structures::vector<uint64_t>();

    // Create the fork process
    int_lk.lock();
    pid = _processes.insert_unique(process());
    process_fork.pid = pid;
    _processes[pid] = process_fork;

    // Add the process as a child process for the parent process
    _processes[process_fork.ppid].child_processes += pid;
    int_lk.unlock();

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
        memory::virtual_allocator::free(kernel_stack, DEFAULT_KERNEL_STACK_SIZE);
        _processes.erase(pid);
        return 0;
    }

    // Create main task for the process
    task = new tcb(thread{.tid = _processes[pid].threads.insert_unique(),
                          .pid = pid,
                          .context = context,
                          .kernel_stack = kernel_stack,
                          .user_stack = user_stack,
                          .args_size = 0,
                          .state = thread_state::ready,
                          .quantum = 0,
                          .sleep_quantum = 0,
                          .child_wait_pid = 0});

    // Set the RIP to return to when the thread is selected
    *(uint64_t *)((uint8_t *)kernel_stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(uint64_t)) =
        (uint64_t)new_fork_process_wrapper;

    // Send to the new user process wrapper the executable object
    segments_obj = new structures::vector<file_segment>(segments);
    old_context_obj = new interrupts::regs(old_context);
    context->rdi = (uint64_t)segments_obj;
    context->rsi = (uint64_t)old_context_obj;

    // Queue the task
    queue_task(task);

    return pid;
}

uint64_t influx::threading::scheduler::sbrk(int64_t inc) {
    interrupts_lock int_lk;

    process &task_process = _processes[_current_task->value().pid];

    // No change
    if (inc == 0) {
        return task_process.program_break_end;
    }

    // Verify the new program break
    if (task_process.program_break_end + inc < task_process.program_break_start ||
        task_process.program_break_end + inc >= DEFAULT_USER_STACK_ADDRESS) {
        return 0;
    }

    // If the the brk section has increased and new pages should be allocated, map new pages
    if (inc > 0 && (task_process.program_break_end / PAGE_SIZE !=
                        (task_process.program_break_end + inc) / PAGE_SIZE ||
                    task_process.program_break_start == task_process.program_break_end)) {
        for (uint64_t addr = task_process.program_break_end +
                             (task_process.program_break_end % PAGE_SIZE
                                  ? (PAGE_SIZE - (task_process.program_break_end % PAGE_SIZE))
                                  : 0);
             addr < (task_process.program_break_end + inc); addr += PAGE_SIZE) {
            // Map new page
            if (!memory::paging_manager::map_page(addr)) {
                return 0;
            }

            // Set page permissions
            memory::paging_manager::set_pte_permissions(addr, PROT_READ | PROT_WRITE, true);
        }
    } else if (inc < 0) {
        // Free all deallocated pages
        for (uint64_t addr =
                 task_process.program_break_end - (task_process.program_break_end % PAGE_SIZE
                                                       ? task_process.program_break_end % PAGE_SIZE
                                                       : PAGE_SIZE);
             addr >= task_process.program_break_end + inc; addr -= PAGE_SIZE) {
            // Free the page
            memory::paging_manager::unmap_page(addr);
        }
    }

    // Set new program break end
    task_process.program_break_end += inc;

    return task_process.program_break_end - inc;
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

    tcb *start_node = nullptr, *currnet_node = nullptr;

    while (true) {
        structures::vector<tcb *> killed_tasks_queue;

        // Create a copy of the current killed tasks queue
        int_lk.lock();
        killed_tasks_queue = _killed_tasks_queue;
        _killed_tasks_queue.clear();
        int_lk.unlock();

        // For each task to be killed
        for (tcb *&task : killed_tasks_queue) {
            bool waited = false;

            // Create a copy of the process struct of the task
            int_lk.lock();
            process &task_process = _processes[task->value().pid];
            int_lk.unlock();

            // Remove the thread from the threads list of the process
            int_lk.lock();
            task_process.threads.erase(algorithm::find(
                task_process.threads.begin(), task_process.threads.end(), task->value().tid));
            int_lk.unlock();

            // Free the task kernel stack
            // NOTE: User stack is already released by paging manager
            memory::virtual_allocator::free(task->value().kernel_stack, DEFAULT_KERNEL_STACK_SIZE);

            // Free the task object
            delete task;

            // If the process has no threads left, kill the process
            if (task_process.threads.empty()) {
                if (!task_process.system) {
                    // Free PML4T
                    memory::virtual_allocator::free(task_process.pml4t, PAGE_SIZE);
                }

                // Move all child processes to init's child processes
                int_lk.lock();
                for (const auto &child_pid : task_process.child_processes) {
                    // If it was already terminated (zombie process), remove it from the list
                    if (_processes[child_pid].terminated) {
                        _processes.erase(child_pid);
                    } else {
                        _processes[INIT_PROCESS_PID].child_processes += child_pid;
                        _processes[child_pid].ppid = INIT_PROCESS_PID;
                    }
                }
                int_lk.unlock();

                // Check if one of the parent's tasks are waiting for the child
                if (task_process.ppid != task_process.pid) {
                    // Get the start node of the priority queue of the parent process
                    int_lk.lock();
                    start_node = _priority_queues[_processes[task_process.ppid].priority].start;
                    currnet_node = start_node;

                    // Search for the tasks of the parent process
                    if (currnet_node != nullptr) {
                        do {
                            // If the task belongs to the parent process and it's waiting for a
                            // child process, check if the process is the wanted process
                            if (currnet_node->value().pid == task_process.ppid &&
                                currnet_node->value().state == thread_state::waiting_for_child) {
                                if (currnet_node->value().child_wait_pid ==
                                        (int64_t)task_process.pid ||
                                    currnet_node->value().child_wait_pid == WAIT_FOR_ANY_PROCESS) {
                                    // Set the pid of the process that released it
                                    currnet_node->value().child_wait_pid = task_process.pid;

                                    // Remove the child as a child process
                                    _processes[task_process.ppid].child_processes.erase(
                                        algorithm::find(
                                            _processes[task_process.ppid].child_processes.begin(),
                                            _processes[task_process.ppid].child_processes.end(),
                                            task_process.pid));

                                    // Release interrupts lock
                                    int_lk.unlock();

                                    // Unblock the task
                                    unblock_task(currnet_node);

                                    // Mark the process as waited
                                    waited = true;
                                    break;
                                }
                            }

                            // Move to next node
                            currnet_node = currnet_node->next();
                        } while (currnet_node != start_node);
                    }
                }

                // If the process was waited or it was executed by init process, delete the process
                // object
                if (waited || task_process.ppid == INIT_PROCESS_PID) {
                    int_lk.lock();
                    _processes[task_process.ppid].child_processes.erase(algorithm::find(
                        _processes[task_process.ppid].child_processes.begin(),
                        _processes[task_process.ppid].child_processes.end(), task_process.pid));
                    _processes.erase(task_process.pid);
                    int_lk.unlock();

                    _log("Process '%s' (PID: %d) has exited.\n", task_process.name.c_str(),
                         task_process.pid);
                } else {
                    // Set the process as terminated and waiting to be deleted (zombie process)
                    task_process.terminated = true;
                }
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

uint64_t influx::threading::scheduler::start_process(influx::threading::executable &exec) {
    interrupts_lock int_lk(false);

    uint64_t pid = 0;
    pml4e_t *pml4t = 0;

    void *kernel_stack = nullptr;
    void *user_stack = nullptr;
    regs *context = nullptr;
    tcb *task = nullptr;

    executable *exec_copy = nullptr;

    // If the file wasn't parsed
    if (!exec.file.parsed()) {
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
    int_lk.lock();
    pid = _processes.insert_unique(process());
    _processes[pid] = process{.pid = pid,
                              .ppid = _current_task->value().pid,
                              .priority = DEFAULT_USER_SPACE_PROCESS_PRIORITY,
                              .system = false,
                              .cr3 = memory::paging_manager::get_physical_address((uint64_t)pml4t),
                              .pml4t = pml4t,
                              .program_break_start = 0,
                              .program_break_end = 0,
                              .threads = structures::unique_vector(),
                              .child_processes = structures::vector<uint64_t>(),
                              .file_descriptors = structures::unique_hash_map<vfs::open_file>(),
                              .name = exec.name,
                              .segments = structures::vector<segment>(),
                              .terminated = false};

    // Add the process as a child process for the current process
    _processes[_current_task->value().pid].child_processes += pid;
    int_lk.unlock();

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
        memory::virtual_allocator::free(kernel_stack, DEFAULT_KERNEL_STACK_SIZE);
        _processes.erase(pid);
        return 0;
    }

    // Create main task for the process
    task = new tcb(thread{.tid = _processes[pid].threads.insert_unique(),
                          .pid = pid,
                          .context = context,
                          .kernel_stack = kernel_stack,
                          .user_stack = user_stack,
                          .args_size = 0,
                          .state = thread_state::ready,
                          .quantum = 0,
                          .sleep_quantum = 0,
                          .child_wait_pid = 0});

    // Set the RIP to return to when the thread is selected
    *(uint64_t *)((uint8_t *)kernel_stack + DEFAULT_KERNEL_STACK_SIZE - sizeof(uint64_t)) =
        (uint64_t)new_user_process_wrapper;

    // Send to the new user process wrapper the executable object
    exec_copy = new executable(exec);
    context->rdi = (uint64_t)exec_copy;

    // Queue the task
    queue_task(task);

    return pid;
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
    if ((new_task_priority_queue.next_task == nullptr ||
         new_task_priority_queue.next_task == _current_task) &&
        task->value().state == thread_state::ready) {
        new_task_priority_queue.next_task = task;
    }
}

uint64_t influx::threading::scheduler::get_stack_pointer() const {
    uint64_t rsp;

    __asm__ __volatile__("mov %0, rsp" : "=r"(rsp));

    return rsp;
}

uint64_t influx::threading::scheduler::pages_for_argv_envp(influx::threading::executable &exec) {
    uint64_t size = 0;

    // Add to size each argument size
    for (const auto &arg : exec.args) {
        size += arg.size() + 1;  // + null char
    }

    // Add to size each enviroment variable size
    for (const auto &env_var : exec.env) {
        size += env_var.size() + 1;  // + null char
    }

    // Add to size the array of args and the array of env
    size += (exec.args.size() + 1 + exec.env.size() + 1) * sizeof(const char **);

    return size / PAGE_SIZE + (size % PAGE_SIZE ? 1 : 0);
}

influx::structures::pair<const char **, const char **> influx::threading::scheduler::copy_argv_envp(
    influx::threading::executable &exec, uint64_t address) {
    uint64_t offset = 0;

    influx::structures::pair<const char **, const char **> ptrs;

    structures::vector<const char *> args_ptrs;
    structures::vector<const char *> env_ptrs;

    // Copy each argument
    for (const auto &arg : exec.args) {
        memory::utils::memcpy((void *)(address + offset), arg.c_str(), arg.size());
        *(uint8_t *)(address + offset + arg.size()) = 0;  // Set null

        // Save arg pointer
        args_ptrs.push_back((const char *)(address + offset));

        // Increase offset
        offset += arg.size() + 1;
    }

    // Copy each environment variable
    for (const auto &env_var : exec.env) {
        memory::utils::memcpy((void *)(address + offset), env_var.c_str(), env_var.size());
        *(uint8_t *)(address + offset + env_var.size()) = 0;  // Set null

        // Save env var pointer
        env_ptrs.push_back((const char *)(address + offset));

        // Increase offset
        offset += env_var.size() + 1;
    }

    // Copy args ptrs
    ptrs.first = (const char **)(address + offset);  // Save arguments ptr
    memory::utils::memcpy((void *)(address + offset), args_ptrs.data(),
                          args_ptrs.size() * sizeof(const char *));
    *(const char **)(address + offset + args_ptrs.size() * sizeof(const char *)) =
        nullptr;                                              // Set null
    offset += (args_ptrs.size() + 1) * sizeof(const char *);  // Increase offset

    // Copy env ptrs
    ptrs.second = (const char **)(address + offset);  // Save arguments ptr
    memory::utils::memcpy((void *)(address + offset), env_ptrs.data(),
                          env_ptrs.size() * sizeof(const char *));
    *(const char **)(address + offset + env_ptrs.size() * sizeof(const char *)) =
        nullptr;                                             // Set null
    offset += (env_ptrs.size() + 1) * sizeof(const char *);  // Increase offset

    return ptrs;
}