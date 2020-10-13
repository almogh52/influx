#include <kernel/threading/scheduler.h>

#include <kernel/algorithm.h>
#include <kernel/assert.h>
#include <kernel/kernel.h>
#include <kernel/memory/paging_manager.h>
#include <kernel/memory/virtual_allocator.h>
#include <kernel/threading/interrupts_lock.h>
#include <kernel/threading/scheduler_started.h>
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

    uint64_t user_stack_virtual_address =
        (uint64_t)kernel::scheduler()->_current_task->value().user_stack;

    uint64_t argv_envp_pages = kernel::scheduler()->_current_task->value().args_size / PAGE_SIZE;

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

    // Map user stack
    for (uint64_t stack_offset = 0; stack_offset < DEFAULT_USER_STACK_SIZE;
         stack_offset += PAGE_SIZE) {
        if (!memory::paging_manager::map_page(
                DEFAULT_USER_STACK_ADDRESS + stack_offset - (argv_envp_pages * PAGE_SIZE),
                memory::paging_manager::get_physical_address(user_stack_virtual_address +
                                                             stack_offset) /
                    PAGE_SIZE)) {
            kernel::scheduler()->kill_current_task();
        }

        // Set R/W permission and DPL of ring 3
        memory::paging_manager::set_pte_permissions(
            DEFAULT_USER_STACK_ADDRESS + stack_offset - (argv_envp_pages * PAGE_SIZE),
            PROT_READ | PROT_WRITE, true);
    }

    // Return to the new process
    scheduler_utils::return_to_fork_process(old_context_var);
}

void influx::threading::terminate_thread() {
    // Kill the current task
    kernel::scheduler()->kill_current_task();
}

influx::threading::scheduler::scheduler(uint64_t tss_addr)
    : _log("Scheduler", console_color::blue),
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
                              .alarm_timer = 0,
                              .working_dir = "/",
                              .threads = structures::unique_vector(),
                              .child_processes = structures::vector<uint64_t>(),
                              .open_files = structures::unique_hash_map<vfs::open_file>(),
                              .file_descriptors = structures::unique_hash_map<vfs::file_descriptor>(
                                  vfs::file_descriptor{.open_file_index = UINT64_MAX}),
                              .name = "kernel",
                              .segments = structures::vector<segment>(),
                              .signal_dispositions = create_default_signal_dispositions(),
                              .pending_std_signals = structures::vector<signal_info>(),
                              .tty = KERNEL_TTY,
                              .exit_code = CLD_EXITED,
                              .exit_status = 0,
                              .terminated = false,
                              .new_exec_process = false});

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
                       .child_wait_pid = 0,
                       .signal_interruptible = false,
                       .signal_interrupted = false,
                       .sig_queue = structures::vector<signal_info>(),
                       .current_sig = SIGINVL,
                       .sig_mask = 0,
                       .old_interrupt_regs = {},
                       .old_sig_mask = 0});
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
                                .child_wait_pid = 0,
                                .signal_interruptible = false,
                                .signal_interrupted = false,
                                .sig_queue = structures::vector<signal_info>(),
                                .current_sig = SIGINVL,
                                .sig_mask = 0,
                                .old_interrupt_regs = {},
                                .old_sig_mask = 0});
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
                              .alarm_timer = 0,
                              .working_dir = "/",
                              .threads = structures::unique_vector(),
                              .child_processes = structures::vector<uint64_t>(),
                              .open_files = structures::unique_hash_map<vfs::open_file>(),
                              .file_descriptors = structures::unique_hash_map<vfs::file_descriptor>(
                                  vfs::file_descriptor{.open_file_index = UINT64_MAX}),
                              .name = "init",
                              .segments = structures::vector<segment>(),
                              .signal_dispositions = create_default_signal_dispositions(),
                              .pending_std_signals = structures::vector<signal_info>(),
                              .tty = INIT_PROCESS_TTY,
                              .exit_code = CLD_EXITED,
                              .exit_status = 0,
                              .terminated = false,
                              .new_exec_process = false});

    // Start init process
    _log("Starting init process..\n");
    _init_process.start();

    // Register tick handler
    _log("Registering tick handler..\n");
    kernel::time_manager()->register_tick_handler(
        utils::method_function_wrapper<scheduler, &scheduler::tick_handler>, this);

    // Set the scheduler as started
    scheduler_started = true;
}

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
                                   .child_wait_pid = 0,
                                   .signal_interruptible = false,
                                   .signal_interrupted = false,
                                   .sig_queue = structures::vector<signal_info>(),
                                   .current_sig = SIGINVL,
                                   .sig_mask = 0,
                                   .old_interrupt_regs = {},
                                   .old_sig_mask = 0});
    int_lk.unlock();

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

    // Update alarm timers
    update_alarm_timers();

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

void influx::threading::scheduler::update_alarm_timers() {
    interrupts_lock int_lk;

    // For each process
    for (auto &process : _processes) {
        // If an alarm timer is running
        if (process.second.alarm_timer > 0 &&
            (process.second.alarm_timer -= (kernel::time_manager()->timer_frequency() / 1000)) ==
                0) {
            // Send SIGALRM to the process
            int_lk.unlock();
            send_signal_to_process(process.first, -1,
                                   signal_info{.sig = SIGALRM,
                                               .error = 0,
                                               .code = 0,
                                               .pid = 0,
                                               .uid = 0,
                                               .status = 0,
                                               .addr = nullptr,
                                               .value_int = 0,
                                               .value_ptr = 0,
                                               .pad = {0}});
            int_lk.lock();
        }
    }
}

uint64_t influx::threading::scheduler::sleep(uint64_t ms) {
    kassert(ms != 0);

    interrupts_lock int_lk;

    process &current_process = _processes[_current_task->value().pid];
    priority_tcb_queue &task_priority_queue = _priority_queues[current_process.priority];

    // Set the task as interruptible
    _current_task->value().signal_interruptible = true;

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

    // Set the task as not interruptible
    _current_task->value().signal_interruptible = false;

    // Return the remaining ms
    return _current_task->value().sleep_quantum /
           (kernel::time_manager()->timer_frequency() / 1000);
}

int64_t influx::threading::scheduler::wait_for_child(int64_t child_pid, uint16_t *wait_status,
                                                     bool no_hang) {
    // ** -1 for all children **

    // Verify child pid
    if (child_pid < WAIT_FOR_ANY_PROCESS || child_pid == 0) {
        return -1;
    }

    interrupts_lock int_lk;

    process &current_process = _processes[_current_task->value().pid];
    priority_tcb_queue &task_priority_queue = _priority_queues[current_process.priority];

    // If the task was interrupted
    if (_current_task->value().signal_interrupted) {
        return -1;
    }

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
        // If the process was killed with a signal
        if (_processes[child_pid].exit_code != CLD_EXITED) {
            _log("Process '%s' (PID: %d) was killed using signal %d.\n",
                 _processes[child_pid].name.c_str(), child_pid, _processes[child_pid].exit_status);
        } else {
            _log("Process '%s' (PID: %d) has exited with code %d.\n",
                 _processes[child_pid].name.c_str(), child_pid, _processes[child_pid].exit_status);
        }

        // Create wait status
        create_wait_status(wait_status, _processes[child_pid]);

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
                // If the process was killed with a signal
                if (_processes[pid].exit_code != CLD_EXITED) {
                    _log("Process '%s' (PID: %d) was killed using signal %d.\n",
                         _processes[pid].name.c_str(), pid, _processes[pid].exit_status);
                } else {
                    _log("Process '%s' (PID: %d) has exited with code %d.\n",
                         _processes[pid].name.c_str(), pid, _processes[pid].exit_status);
                }

                // Create wait status
                create_wait_status(wait_status, _processes[pid]);

                // Remove the process object
                _processes.erase(pid);

                // Remove the process as a child prcess
                current_process.child_processes.erase(&pid);

                return pid;
            }
        }
    }

    // If no hang, return
    if (no_hang) {
        return 0;
    }

    // If the task isn't interrupted yet
    if (!_current_task->value().signal_interrupted) {
        // Set the child pid that the task is waiting for
        _current_task->value().child_wait_pid = child_pid;

        // Set the task as interruptible
        _current_task->value().signal_interruptible = true;

        // Set the task's state to waiting
        _current_task->value().state = thread_state::waiting_for_child;

        // If it is the next task, set the next task as null
        if (task_priority_queue.next_task == _current_task) {
            task_priority_queue.next_task = nullptr;
        }
        int_lk.unlock();

        // Re-schedule to another task
        reschedule();
    }

    // If the task was interrupted
    if (_current_task->value().signal_interrupted) {
        return -1;
    }

    // Create wait status
    create_wait_status(wait_status, _processes[_current_task->value().child_wait_pid]);

    // Remove process
    int_lk.lock();
    _processes.erase(_current_task->value().child_wait_pid);
    int_lk.unlock();

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
        task->value().state == thread_state::sleeping ||
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

void influx::threading::scheduler::exit(uint8_t code) {
    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];
    int_lk.unlock();

    // Set the error code
    process.exit_code = CLD_EXITED;
    process.exit_status = code;

    // Kill all tasks of the process
    kill_all_tasks(_current_task->value().pid);
}

uint64_t influx::threading::scheduler::exec(
    size_t fd, const influx::structures::string &name,
    const influx::structures::vector<influx::structures::string> &args,
    const influx::structures::vector<influx::structures::string> &env) {
    vfs::file_info file;
    executable exec{.name = name, .file = elf_file(fd), .args = args, .env = env};

    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];
    int_lk.unlock();

    // Check that the file has permission to execute
    if (kernel::vfs()->stat(fd, file) != vfs::error::success || !file.permissions.execute) {
        return 0;
    }

    // Try to parse the file as an ELF file
    if (!exec.file.parse()) {
        // TODO: Search for interpreter line
        return 0;
    }

    // If the kernel is requesting to start a process, queue it in the init process
    if (_current_task->value().pid == KERNEL_PID) {
        _init_process.queue_exec(exec);
        return INIT_PROCESS_PID;
    }

    // TODO: Kill all other threads

    // Set the process as new exec process
    process.new_exec_process = true;

    // Clean the process
    clean_process(_current_task->value().pid, false, false);

    // Start the process
    start_process(exec, _current_task->value().pid);

    // Kill the current task
    kill_current_task();

    return 0;
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
    process &parent_process = _processes[_current_task->value().pid];
    int_lk.unlock();

    // Allocate PML4T
    pml4t = (pml4e_t *)memory::virtual_allocator::allocate(PAGE_SIZE, PROT_READ | PROT_WRITE);
    if (pml4t == 0) {
        return 0;
    }

    // Initiate PML4T for the userland executable
    memory::paging_manager::init_user_process_paging((uint64_t)pml4t);

    // Create a copy of all the segments of the current process
    for (const auto &exec_seg : parent_process.segments) {
        seg = file_segment{.virtual_address = exec_seg.virtual_address,
                           .data = structures::dynamic_buffer(exec_seg.size),
                           .protection = exec_seg.protection};

        // Copy the segment data
        memory::utils::memcpy(seg.data.data(), (void *)exec_seg.virtual_address, exec_seg.size);

        // Add the segment to the segments vector
        segments += seg;
    }

    // Create a copy of the program break
    seg = file_segment{.virtual_address = parent_process.program_break_start,
                       .data = structures::dynamic_buffer(parent_process.program_break_end -
                                                          parent_process.program_break_start),
                       .protection = PROT_READ | PROT_WRITE};
    memory::utils::memcpy(seg.data.data(), (void *)parent_process.program_break_start,
                          parent_process.program_break_end - parent_process.program_break_start);
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
    kernel::vfs()->fork_file_descriptors(parent_process.open_files);

    // Create the fork process
    int_lk.lock();
    pid = _processes.insert_unique(
        process{.pid = 0,
                .ppid = parent_process.pid,
                .priority = parent_process.priority,
                .system = parent_process.system,
                .cr3 = memory::paging_manager::get_physical_address((uint64_t)pml4t),
                .pml4t = pml4t,
                .program_break_start = parent_process.program_break_start,
                .program_break_end = parent_process.program_break_end,
                .alarm_timer = 0,
                .working_dir = parent_process.working_dir,
                .threads = structures::unique_vector(),
                .child_processes = structures::vector<uint64_t>(),
                .open_files = structures::unique_hash_map<vfs::open_file>(),
                .file_descriptors = structures::unique_hash_map<vfs::file_descriptor>(
                    vfs::file_descriptor{.open_file_index = UINT64_MAX}),
                .name = parent_process.name,
                .segments = parent_process.segments,
                .signal_dispositions = parent_process.signal_dispositions,
                .pending_std_signals = structures::vector<signal_info>(),
                .tty = parent_process.tty,
                .exit_code = CLD_EXITED,
                .exit_status = 0,
                .terminated = false,
                .new_exec_process = false});
    _processes[pid].pid = pid;

    // Recreate file descriptors
    for (auto &fd : parent_process.file_descriptors) {
        _processes[pid].file_descriptors[fd.first] = fd.second;
    }

    // Add the process as a child process for the parent process
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
    memory::utils::memcpy(user_stack, _current_task->value().user_stack, DEFAULT_USER_STACK_SIZE);

    // Create main task for the process
    int_lk.lock();
    task = new tcb(thread{.tid = _processes[pid].threads.insert_unique(),
                          .pid = pid,
                          .context = context,
                          .kernel_stack = kernel_stack,
                          .user_stack = user_stack,
                          .args_size = _current_task->value().args_size,
                          .state = thread_state::ready,
                          .quantum = 0,
                          .sleep_quantum = 0,
                          .child_wait_pid = 0,
                          .signal_interruptible = false,
                          .signal_interrupted = false,
                          .sig_queue = structures::vector<signal_info>(),
                          .current_sig = SIGINVL,
                          .sig_mask = _current_task->value().sig_mask,
                          .old_interrupt_regs = {},
                          .old_sig_mask = 0});
    int_lk.unlock();

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

uint64_t influx::threading::scheduler::alarm(uint64_t ms) {
    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];

    uint64_t old_ms = process.alarm_timer / (kernel::time_manager()->timer_frequency() / 1000);

    process.alarm_timer = ms * (kernel::time_manager()->timer_frequency() / 1000);

    return old_ms;
}

influx::vfs::path influx::threading::scheduler::get_working_dir() {
    interrupts_lock int_lk;
    return _processes[_current_task->value().pid].working_dir;
}

int64_t influx::threading::scheduler::set_working_dir(influx::vfs::path dir) {
    vfs::file_info info;
    int64_t err;

    interrupts_lock int_lk;

    process &process = _processes[_current_task->value().pid];

    // If the directory is relative, make it absolute using current working dir
    if (dir.is_relative()) {
        dir = process.working_dir + dir;
    }

    // Try to get the stat of the file
    if ((err = kernel::vfs()->stat(dir, info)) != vfs::error::success) {
        return err;
    }

    // If the file isn't a directory
    if (info.type != vfs::file_type::directory) {
        return vfs::error::file_is_not_directory;
    }

    // Set the new working dir
    process.working_dir = dir;

    return 0;
}

influx::threading::signal_action influx::threading::scheduler::get_signal_action(
    influx::threading::signal sig) {
    interrupts_lock int_lk;
    return _processes[_current_task->value().pid].signal_dispositions[sig];
}

void influx::threading::scheduler::set_signal_action(influx::threading::signal sig,
                                                     influx::threading::signal_action action) {
    interrupts_lock int_lk;

    // Can't catch SIGKILL and SIGSTOP
    if (sig == SIGKILL || sig == SIGSTOP || sig < 1 || sig >= SIGSTD_N) {
        return;
    }

    // Verify that there is a restorer in case of a handler
    if (action.handler.raw > 1 && action.restorer == nullptr) {
        return;
    }

    // Set signal disposition
    _processes[_current_task->value().pid].signal_dispositions[sig] = action;
}

bool influx::threading::scheduler::send_signal(int64_t pid, int64_t tid,
                                               influx::threading::signal_info sig_info) {
    interrupts_lock int_lk(false);
    structures::vector<uint64_t> prcoesses;

    // Send to all process in process group
    if (pid == 0) {
        // TODO: Implement
        return false;
    }
    // Send to all process in a specific process group
    else if (pid < -1) {
        // TODO: Implement
        return false;
    } else if (pid == INIT_PROCESS_PID) {
        return false;
    }

    // Send signal to all processes
    if (pid == -1) {
        // Create a list of all processes
        int_lk.lock();
        for (auto &proc : _processes) {
            // If the proccess isn't the kernel, the init process or the calling process
            if (proc.first != KERNEL_PID && proc.first != INIT_PROCESS_PID &&
                proc.first != _current_task->value().pid) {
                prcoesses.push_back(proc.first);
            }
        }
        int_lk.unlock();

        // For each process, send the signal
        if (sig_info.sig != 0) {
            for (const auto &pid : prcoesses) {
                send_signal_to_process(pid, -1, sig_info);
            }
        }
    } else {
        // Check that process and the thread exists
        int_lk.lock();
        if (_processes.count(pid) == 0 ||
            (tid != -1 &&
             algorithm::find(_processes[pid].threads.begin(), _processes[pid].threads.end(),
                             (uint64_t)tid) == _processes[pid].threads.end())) {
            return false;
        }
        int_lk.unlock();

        // Send the signal to the process
        if (sig_info.sig != 0) {
            send_signal_to_process((uint64_t)pid, tid, sig_info);
        }
    }

    return true;
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

uint64_t influx::threading::scheduler::get_current_parent_process_id() {
    interrupts_lock int_lk;
    return _processes[_current_task->value().pid].ppid;
}

bool influx::threading::scheduler::interrupted() const {
    return _current_task->value().signal_interrupted;
}

uint64_t influx::threading::scheduler::add_file_descriptor(const influx::vfs::open_file &file) {
    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];

    uint64_t open_file_index = process.open_files.insert_unique(file);

    return process.file_descriptors.insert_unique(
        vfs::file_descriptor{.open_file_index = open_file_index});
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
    file = process.open_files[process.file_descriptors[fd].open_file_index];

    return vfs::error::success;
}

void influx::threading::scheduler::update_file_descriptor(uint64_t fd,
                                                          influx::vfs::open_file &file) {
    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];

    process.open_files[process.file_descriptors[fd].open_file_index] = file;
}

void influx::threading::scheduler::remove_file_descriptor(uint64_t fd) {
    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];

    // If the open file has no file descriptors, delete it
    if (--process.open_files[process.file_descriptors[fd].open_file_index]
              .amount_of_file_descriptors == 0) {
        process.open_files.erase(process.file_descriptors[fd].open_file_index);
    }

    process.file_descriptors.erase(fd);
}

uint64_t influx::threading::scheduler::duplicate_file_descriptor(uint64_t oldfd, int64_t newfd) {
    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];

    vfs::file_descriptor file_descriptor = process.file_descriptors[oldfd];
    vfs::open_file &file = process.open_files[file_descriptor.open_file_index];

    int_lk.unlock();
    threading::unique_lock vnodes_lk(kernel::vfs()->_vnodes_mutex);
    vfs::vnode &vn = kernel::vfs()->_vnodes[file.vnode_index];
    vnodes_lk.unlock();

    // Call filesystem duplicate function
    vn.fs->duplicate_open_file(file, vn.fs_data);
    int_lk.lock();

    // Increase amount of file descriptors for the open file
    file.amount_of_file_descriptors++;

    // If no new fd was specified
    if (newfd == -1) {
        return process.file_descriptors.insert_unique(file_descriptor);
    } else {
        // If the new fd is in use, close it
        if (process.file_descriptors.count(newfd) != 0) {
            if (--process.open_files[process.file_descriptors[newfd].open_file_index]
                      .amount_of_file_descriptors == 0) {
                process.open_files.erase(process.file_descriptors[newfd].open_file_index);
            }
        }

        process.file_descriptors[newfd] = file_descriptor;
    }

    return newfd;
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
            process &task_process = _processes[task->value().pid];
            int_lk.unlock();

            // Free the task kernel stack
            memory::virtual_allocator::free(task->value().kernel_stack, DEFAULT_KERNEL_STACK_SIZE);

            // Free the task user stack
            if (task->value().user_stack != nullptr) {
                memory::virtual_allocator::free(task->value().user_stack, DEFAULT_USER_STACK_SIZE);
            }

            // Remove the thread from the threads list of the process
            if (!task_process.new_exec_process) {
                int_lk.lock();
                task_process.threads.erase(algorithm::find(
                    task_process.threads.begin(), task_process.threads.end(), task->value().tid));
                int_lk.unlock();
            }

            // Free the task object
            delete task;

            // If the process isn't a new exec process
            if (!task_process.new_exec_process) {
                // If the process has no threads left, kill the process
                if (task_process.threads.empty()) {
                    // Clean the process
                    if (task_process.ppid != task_process.pid) {
                        clean_process(task_process.pid, true, true);
                    }
                }
            } else {
                task_process.new_exec_process = false;
            }
        }

        // Block this task
        block_current_task();
    }
}

void influx::threading::scheduler::idle_task() {
    while (true) {
        __asm__ __volatile__("sti; hlt");  // Wait for interrupt

        // Try to reschedule to another task since probably some task is available
        reschedule();
    }
}

uint64_t influx::threading::scheduler::start_process(influx::threading::executable &exec,
                                                     int64_t pid) {
    interrupts_lock int_lk(false);

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
    if (pid < 0) {
        pid = _processes.insert_unique(process());
        _processes[pid] =
            process{.pid = (uint64_t)pid,
                    .ppid = _current_task->value().pid,
                    .priority = DEFAULT_USER_SPACE_PROCESS_PRIORITY,
                    .system = false,
                    .cr3 = memory::paging_manager::get_physical_address((uint64_t)pml4t),
                    .pml4t = pml4t,
                    .program_break_start = 0,
                    .program_break_end = 0,
                    .alarm_timer = 0,
                    .working_dir = _processes[_current_task->value().pid].working_dir,
                    .threads = structures::unique_vector(),
                    .child_processes = structures::vector<uint64_t>(),
                    .open_files = structures::unique_hash_map<vfs::open_file>(),
                    .file_descriptors = structures::unique_hash_map<vfs::file_descriptor>(
                        vfs::file_descriptor{.open_file_index = UINT64_MAX}),
                    .name = exec.name,
                    .segments = structures::vector<segment>(),
                    .signal_dispositions = create_default_signal_dispositions(),
                    .pending_std_signals = structures::vector<signal_info>(),
                    .tty = _processes[_current_task->value().pid].tty,
                    .exit_code = CLD_EXITED,
                    .exit_status = 0,
                    .terminated = false,
                    .new_exec_process = false};

        // Add the process as a child process for the current process
        _processes[_current_task->value().pid].child_processes += pid;
    } else {
        _processes[pid].cr3 = memory::paging_manager::get_physical_address((uint64_t)pml4t);
        _processes[pid].pml4t = pml4t;
        _processes[pid].program_break_start = 0;
        _processes[pid].program_break_end = 0;
        _processes[pid].threads = structures::unique_vector();
        _processes[pid].child_processes = structures::vector<uint64_t>();
        _processes[pid].name = exec.name;
        _processes[pid].terminated = false;
        _processes[pid].signal_dispositions = create_default_signal_dispositions();
        _processes[pid].pending_std_signals = structures::vector<signal_info>();
    }

    // Create file descriptors for stdin, stdout and stderr
    kernel::vfs()
        ->_vnodes[kernel::tty_manager()->get_tty_vnode(_processes[pid].tty)]
        .amount_of_open_files += 3;
    _processes[pid].open_files.insert_unique(
        vfs::open_file{.vnode_index = kernel::tty_manager()->get_tty_vnode(_processes[pid].tty),
                       .position = 0,
                       .flags = vfs::open_flags::read,
                       .amount_of_file_descriptors = 1});
    _processes[pid].open_files.insert_unique(
        vfs::open_file{.vnode_index = kernel::tty_manager()->get_tty_vnode(_processes[pid].tty),
                       .position = 0,
                       .flags = vfs::open_flags::write,
                       .amount_of_file_descriptors = 1});
    _processes[pid].open_files.insert_unique(
        vfs::open_file{.vnode_index = kernel::tty_manager()->get_tty_vnode(_processes[pid].tty),
                       .position = 0,
                       .flags = vfs::open_flags::write,
                       .amount_of_file_descriptors = 1});
    _processes[pid].file_descriptors.insert_unique(vfs::file_descriptor{.open_file_index = 0});
    _processes[pid].file_descriptors.insert_unique(vfs::file_descriptor{.open_file_index = 1});
    _processes[pid].file_descriptors.insert_unique(vfs::file_descriptor{.open_file_index = 2});
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
                          .pid = (uint64_t)pid,
                          .context = context,
                          .kernel_stack = kernel_stack,
                          .user_stack = user_stack,
                          .args_size = 0,
                          .state = thread_state::ready,
                          .quantum = 0,
                          .sleep_quantum = 0,
                          .child_wait_pid = 0,
                          .signal_interruptible = false,
                          .signal_interrupted = false,
                          .sig_queue = structures::vector<signal_info>(),
                          .current_sig = SIGINVL,
                          .sig_mask = _current_task->value().sig_mask,
                          .old_interrupt_regs = {},
                          .old_sig_mask = 0});

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

void influx::threading::scheduler::clean_process(uint64_t pid, bool erase,
                                                 bool close_file_descriptors) {
    interrupts_lock int_lk(false);

    tcb *start_node = nullptr, *currnet_node = nullptr;
    bool waited = false;

    process &process = _processes[pid];

    if (!process.system) {
        // Free PML4T
        memory::virtual_allocator::free(process.pml4t, PAGE_SIZE);
    }

    // Move all child processes to init's child processes
    int_lk.lock();
    for (const auto &child_pid : process.child_processes) {
        // If it was already terminated (zombie process), remove it from the list
        if (_processes[child_pid].terminated) {
            // If the process was killed with a signal
            if (_processes[child_pid].exit_code != CLD_EXITED) {
                _log("Process '%s' (PID: %d) was killed using signal %d.\n",
                     _processes[child_pid].name.c_str(), child_pid,
                     _processes[child_pid].exit_status);
            } else {
                _log("Process '%s' (PID: %d) has exited with code %d.\n",
                     _processes[child_pid].name.c_str(), child_pid,
                     _processes[child_pid].exit_status);
            }

            _processes.erase(child_pid);
        } else {
            _processes[INIT_PROCESS_PID].child_processes += child_pid;
            _processes[child_pid].ppid = INIT_PROCESS_PID;
        }
    }

    // Don't check for waiting tasks if not erasing process
    if (erase) {
        // Send SIGCHLD to parent process
        send_signal_to_process(process.ppid, -1,
                               signal_info{.sig = SIGCHLD,
                                           .error = 0,
                                           .code = process.exit_code,
                                           .pid = process.pid,
                                           .uid = 0,
                                           .status = process.exit_status,
                                           .addr = nullptr,
                                           .value_int = 0,
                                           .value_ptr = nullptr,
                                           .pad = {0}});

        // Get the start node of the priority queue of the parent process
        start_node = _priority_queues[_processes[process.ppid].priority].start;
        currnet_node = start_node;

        // Search for the tasks of the parent process
        if (currnet_node != nullptr) {
            do {
                // If the task belongs to the parent process and it's waiting for a
                // child process, check if the process is the wanted process
                if (currnet_node->value().pid == process.ppid &&
                    currnet_node->value().state == thread_state::waiting_for_child) {
                    if (currnet_node->value().child_wait_pid == (int64_t)pid ||
                        currnet_node->value().child_wait_pid == WAIT_FOR_ANY_PROCESS) {
                        // Set the pid of the process that released it
                        currnet_node->value().child_wait_pid = pid;

                        // Remove the child as a child process
                        _processes[process.ppid].child_processes.erase(
                            algorithm::find(_processes[process.ppid].child_processes.begin(),
                                            _processes[process.ppid].child_processes.end(), pid));

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

    // Unlock interrupts lock
    if (!waited) {
        int_lk.unlock();
    }

    // Close all open files
    if (close_file_descriptors) {
        for (const auto &file_descriptor : process.open_files) {
            kernel::vfs()->close_open_file(file_descriptor.second);
        }
    }

    // Lock interrupts lock
    int_lk.lock();

    // If the process was waited or it was executed by init process, delete the process
    // object
    if (erase && (waited || process.ppid == INIT_PROCESS_PID ||
                  (_processes[process.ppid].signal_dispositions[SIGCHLD].flags & SA_NOCLDWAIT) ||
                  _processes[process.ppid].signal_dispositions[SIGCHLD].handler.raw == SIG_IGN)) {
        // If the process was killed with a signal
        if (process.exit_code != CLD_EXITED) {
            _log("Process '%s' (PID: %d) was killed using signal %d.\n", process.name.c_str(), pid,
                 process.exit_status);
        } else {
            _log("Process '%s' (PID: %d) has exited with code %d.\n", process.name.c_str(), pid,
                 process.exit_status);
        }

        _processes[process.ppid].child_processes.erase(
            algorithm::find(_processes[process.ppid].child_processes.begin(),
                            _processes[process.ppid].child_processes.end(), pid));
        if (!waited) {
            _processes.erase(pid);
        } else {
            process.terminated = true;
        }
    } else {
        // Set the process as terminated and waiting to be deleted (zombie process)
        process.terminated = true;
    }
}

void influx::threading::scheduler::kill_all_tasks(uint64_t pid) {
    interrupts_lock int_lk;

    kassert(_processes.count(pid));
    process &process = _processes[pid];
    priority_tcb_queue &process_priority_queue = _priority_queues[process.priority];

    tcb *current_node = process_priority_queue.start;

    interrupts::regs *task_regs = nullptr;

    // While we didn't reach the end of the priority queue
    do {
        // If the current node is for the wanted process and it's not the current task, set it to be
        // terminated
        if (current_node->value().pid == pid) {
            // Get the interrupt regs of the task
            task_regs = get_task_interrupt_regs(current_node);

            // Change interrupt return to terminate thread function of the scheduler
            task_regs->cs = 0x8;
            task_regs->ss = 0x10;
            task_regs->rsp =
                (uint64_t)current_node->value().kernel_stack + DEFAULT_KERNEL_STACK_SIZE;
            task_regs->rip = (uint64_t)terminate_thread;

            // Set the task as interrupted
            current_node->value().signal_interrupted = true;

            // If the task is interruptible and it's blocked, interrupt it and unblock it
            if (current_node->value().signal_interruptible &&
                (current_node->value().state == thread_state::blocked ||
                 current_node->value().state == thread_state::sleeping ||
                 current_node->value().state == thread_state::waiting_for_child)) {
                unblock_task(current_node);
            }
        }

        // Move to the next node
        current_node = current_node->next();
    } while (current_node != process_priority_queue.start);
}

void influx::threading::scheduler::kill_with_signal(uint64_t pid, influx::threading::signal sig) {
    interrupts_lock int_lk;

    // Set the kill signal of the process
    _processes[pid].exit_code = CLD_KILLED;
    _processes[pid].exit_status = sig;

    // Kill all the tasks of the process
    kill_all_tasks(pid);
}

void influx::threading::scheduler::queue_task(influx::threading::tcb *task) {
    interrupts_lock int_lk;

    process &new_task_process = _processes[task->value().pid];
    priority_tcb_queue &new_task_priority_queue = _priority_queues[new_task_process.priority];

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

influx::structures::vector<influx::threading::signal_action>
influx::threading::scheduler::create_default_signal_dispositions() const {
    structures::vector<signal_action> signal_dispositions;

    // Create default dispositions for each signal
    for (uint64_t i = 0; i < SIGSTD_N; i++) {
        signal_dispositions.push_back(signal_action{
            .handler = signal_handler{.raw = SIG_DFL}, .mask = 0, .flags = 0, .restorer = nullptr});
    }

    return signal_dispositions;
}

void influx::threading::scheduler::prepare_signal_handle(influx::threading::tcb *task,
                                                         influx::threading::signal_info sig_info) {
    kassert(task->value().current_sig != SIGINVL);

    uint64_t user_stack_start = DEFAULT_USER_STACK_ADDRESS - task->value().args_size;

    interrupts::regs *regs = get_task_interrupt_regs(task);

    // Set the RIP to the handler function and save the old RIP
    task->value().old_interrupt_regs = *regs;
    regs->rip = _processes[task->value().pid].signal_dispositions[sig_info.sig].handler.raw;

    // Copy the signal info structure to the stack
    if (_processes[task->value().pid].signal_dispositions[sig_info.sig].flags & SA_SIGINFO) {
        regs->rsp -= sizeof(signal_info);
        memory::utils::memcpy(
            (void *)((uint64_t)task->value().user_stack + (regs->rsp - user_stack_start)),
            &sig_info, sizeof(signal_info));
        regs->rsi = regs->rsp;
        regs->rdx = 0;  // TODO: Implement user context
    }

    // Set the parameters for the handler function
    regs->rdi = sig_info.sig;

    // Set the restorer function in the user stack
    regs->rsp -= sizeof(uint64_t);
    *((uint64_t *)((uint64_t)task->value().user_stack + (regs->rsp - user_stack_start))) =
        (uint64_t)_processes[task->value().pid].signal_dispositions[sig_info.sig].restorer;
}

void influx::threading::scheduler::send_signal_to_process(uint64_t pid, int64_t tid,
                                                          influx::threading::signal_info sig_info) {
    interrupts_lock int_lk;
    kassert(_processes.count(pid) != 0);

    process &process = _processes[pid];
    priority_tcb_queue &process_priority_queue = _priority_queues[process.priority];

    tcb *current_node = process_priority_queue.start;

    // Check valid process id
    if (pid == 0 || pid == INIT_PROCESS_PID) {
        return;
    }

    // If the signal is the kill or stop signal or the handler is the default signal, call the
    // default handler
    if (sig_info.sig == SIGKILL || sig_info.sig == SIGSTOP ||
        process.signal_dispositions[sig_info.sig].handler.raw == SIG_DFL) {
        int_lk.unlock();
        default_signal_handler(process, sig_info);
        return;
    }

    // If the handler for the signal is to ignore the signal, ignore it
    if (process.signal_dispositions[sig_info.sig].handler.raw == SIG_IGN) {
        return;
    }

    // If the process is a zombie process, add the signal to it's pending signals
    if (process.terminated == true) {
        process.pending_std_signals += sig_info;
        return;
    }

    // If there isn't a wanted thread
    if (tid == -1) {
        // Search for the main thread
        do {
            if (current_node->value().pid == pid && current_node->value().tid == 0) {
                break;
            }

            // Move to the next node
            current_node = current_node->next();
        } while (current_node != process_priority_queue.start);

        // If the main thread can't handle the signal
        if (current_node->value().pid != pid || current_node->value().tid != 0 ||
            (current_node->value().sig_mask & (1 << sig_info.sig)) ||
            current_node->value().current_sig != SIGINVL) {
            // Reset current node
            current_node = process_priority_queue.start;

            // Search for a thread that can handle the signal
            do {
                if (current_node->value().pid == pid &&
                    !(current_node->value().sig_mask & (1 << sig_info.sig)) &&
                    current_node->value().current_sig == SIGINVL) {
                    break;
                }

                // Move to the next node
                current_node = current_node->next();
            } while (current_node != process_priority_queue.start);

            // If the node can't handle the signal
            if (current_node->value().pid != pid ||
                (current_node->value().sig_mask & (1 << sig_info.sig)) ||
                current_node->value().current_sig != SIGINVL) {
                current_node = nullptr;
            }
        }

        // If no thread can handle the signal, queue it
        if (current_node == nullptr) {
            process.pending_std_signals += sig_info;
        } else {
            send_signal_to_task(current_node, sig_info);
        }
    } else {
        // Search for the thread
        do {
            // Check if it's the wanted thread
            if (current_node->value().pid == pid && current_node->value().tid == (uint64_t)tid) {
                // If the thread blocks the signal or is busy with another signal
                if ((current_node->value().sig_mask & (1 << sig_info.sig)) ||
                    current_node->value().current_sig != SIGINVL) {
                    current_node->value().sig_queue += sig_info;
                } else {
                    send_signal_to_task(current_node, sig_info);
                }
            }

            // Move to the next node
            current_node = current_node->next();
        } while (current_node != process_priority_queue.start);
    }
}

void influx::threading::scheduler::send_signal_to_task(influx::threading::tcb *task,
                                                       influx::threading::signal_info sig_info) {
    // ** Interrupts should be locked here **
    kassert(task->value().current_sig == SIGINVL);

    process &process = _processes[task->value().pid];

    // Set the current signal
    task->value().current_sig = sig_info.sig;

    // Save the current signal mask and mask it using the signal's disposition mask
    task->value().old_sig_mask = task->value().sig_mask;
    task->value().sig_mask |=
        process.signal_dispositions[sig_info.sig].mask |
        ((process.signal_dispositions[sig_info.sig].flags & SA_NODEFER) ? 0 : (1 << sig_info.sig));

    // Prepare the signal handler
    prepare_signal_handle(task, sig_info);

    // Reset to default handler, if requested
    if (process.signal_dispositions[sig_info.sig].flags & SA_RESETHAND) {
        process.signal_dispositions[sig_info.sig].handler.raw = SIG_DFL;
    }

    // Set the task as interrupted
    task->value().signal_interrupted = true;

    // If the task is interruptible and it's blocked, interrupt it and unblock it
    if (task->value().signal_interruptible &&
        (task->value().state == thread_state::blocked ||
         task->value().state == thread_state::sleeping ||
         task->value().state == thread_state::waiting_for_child)) {
        unblock_task(task);
    }
}

void influx::threading::scheduler::default_signal_handler(influx::threading::process &process,
                                                          influx::threading::signal_info sig_info) {
    // TODO: Implement process stopping and cotinuing (SIGSTOP, SIGCONT ...)

    // Default signal action for ignoring
    if (sig_info.sig == SIGCHLD || sig_info.sig == SIGURG || sig_info.sig == SIGWINCH) {
        return;
    }

    // Dump core
    if (sig_info.sig == SIGABRT || sig_info.sig == SIGBUS || sig_info.sig == SIGFPE ||
        sig_info.sig == SIGILL || sig_info.sig == SIGIOT || sig_info.sig == SIGQUIT ||
        sig_info.sig == SIGSEGV || sig_info.sig == SIGSYS || sig_info.sig == SIGTRAP ||
        sig_info.sig == SIGXCPU || sig_info.sig == SIGXFSZ) {
        // TODO: Implement core dump
    }

    // If the process isn't terminated, terminate it
    if (!process.terminated) {
        kill_with_signal(process.pid, sig_info.sig);
    }
}

bool influx::threading::scheduler::get_next_signal_info(influx::threading::signal_info &sig_info) {
    // ** Interrupts should be locked here **
    process &process = _processes[_current_task->value().pid];

    signal_mask mask = _current_task->value().sig_mask;

    // For each pending signal in the task sig queue, check if it's not blocked
    for (auto it = _current_task->value().sig_queue.begin();
         it != _current_task->value().sig_queue.end(); it++) {
        if (!(mask & (1 << it->sig))) {
            sig_info = *it;
            _current_task->value().sig_queue.erase(it);
            return true;
        }
    }

    // For each pending signal in the process sig queue, check if it's not blocked
    for (auto it = process.pending_std_signals.begin(); it != process.pending_std_signals.end();
         it++) {
        if (!(mask & (1 << it->sig))) {
            sig_info = *it;
            process.pending_std_signals.erase(it);
            return true;
        }
    }

    return false;
}

void influx::threading::scheduler::handle_signal_return(influx::interrupts::regs *context) {
    interrupts_lock int_lk;
    kassert(_current_task->value().current_sig != SIGINVL);

    process &process = _processes[_current_task->value().pid];

    signal_info sig_info;
    sig_info.sig = SIGINVL;

    // Restore old regs
    *context = _current_task->value().old_interrupt_regs;

    // Restore old signal mask
    _current_task->value().sig_mask = _current_task->value().old_sig_mask;

    // Set the task as not interrupted
    _current_task->value().signal_interruptible = false;
    _current_task->value().signal_interrupted = false;

    // Set the current signal as invalid
    _current_task->value().current_sig = SIGINVL;

    // If there is another signal to handle, prepare the signal handler
    if (get_next_signal_info(sig_info)) {
        // Set the current signal
        _current_task->value().current_sig = sig_info.sig;

        // Save the current signal mask and mask it using the signal's disposition mask
        _current_task->value().old_sig_mask = _current_task->value().sig_mask;
        _current_task->value().sig_mask |= process.signal_dispositions[sig_info.sig].mask;

        // Prepare the signal handler
        prepare_signal_handle(_current_task, sig_info);
    }
}

influx::threading::signal_mask influx::threading::scheduler::get_signal_mask() {
    return _current_task->value().sig_mask;
}

void influx::threading::scheduler::set_signal_mask(influx::threading::signal_mask mask) {
    signal_info sig_info;
    sig_info.sig = SIGINVL;

    interrupts_lock int_lk;
    process &process = _processes[_current_task->value().pid];

    // Set the new signal mask
    _current_task->value().sig_mask = mask;

    // If there is another signal to handle, prepare the signal handler
    if (_current_task->value().current_sig == SIGINVL && get_next_signal_info(sig_info)) {
        // Set the current signal
        _current_task->value().current_sig = sig_info.sig;

        // Save the current signal mask and mask it using the signal's disposition mask
        _current_task->value().old_sig_mask = _current_task->value().sig_mask;
        _current_task->value().sig_mask |= process.signal_dispositions[sig_info.sig].mask;

        // Prepare the signal handler
        prepare_signal_handle(_current_task, sig_info);
    }
}

void influx::threading::scheduler::send_sigint_to_tty(uint64_t tty) {
    structures::vector<uint64_t> tty_processes;

    signal_info sig_info = {.sig = SIGINT,
                            .error = 0,
                            .code = 0,
                            .pid = KERNEL_PID,
                            .uid = 0,
                            .status = 0,
                            .addr = nullptr,
                            .value_int = 0,
                            .value_ptr = 0,
                            .pad = {0}};

    interrupts_lock int_lk;

    // Get all processes of TTY
    for (auto &process : _processes) {
        if (process.second.tty == tty) {
            tty_processes.push_back(process.first);
        }
    }
    int_lk.unlock();

    // Send SIGINT to each process
    for (const auto &pid : tty_processes) {
        send_signal_to_process(pid, -1, sig_info);
    }
}

influx::interrupts::regs *influx::threading::scheduler::get_task_interrupt_regs(
    influx::threading::tcb *task) {
    uint64_t *kernel_stack_ptr =
        task == _current_task ? (uint64_t *)get_stack_pointer() : (uint64_t *)task->value().context;

    // Find the interrupt regs
    while (*kernel_stack_ptr != 0x1B || *(kernel_stack_ptr + 3) != 0x23) {
        kernel_stack_ptr++;
    }

    // Calc the interrupt regs pointer
    return (interrupts::regs *)(kernel_stack_ptr -
                                (sizeof(interrupts::regs) / sizeof(uint64_t) - 4));
}

void influx::threading::scheduler::create_wait_status(uint16_t *wait_status,
                                                      influx::threading::process &process) {
    kassert(process.terminated);

    // If the process exited
    if (process.exit_code == CLD_EXITED) {
        *wait_status = (uint16_t)((process.exit_status) << 8);
    } else if (process.exit_code == CLD_KILLED) {  // Process killed by signal
        *wait_status = (uint16_t)process.exit_status;
    } else if (process.exit_code == CLD_STOPPED) {  // Process stopped by signal
        *wait_status = (uint16_t)(0x7f + ((process.exit_status) << 8));
    } else {
        *wait_status = 0;
    }
}