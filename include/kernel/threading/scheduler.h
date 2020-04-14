#pragma once
#include <kernel/elf_file.h>
#include <kernel/logger.h>
#include <kernel/memory/memory.h>
#include <kernel/structures/unique_hash_map.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/init_process.h>
#include <kernel/threading/process.h>
#include <kernel/threading/thread.h>
#include <kernel/vfs/error.h>
#include <kernel/vfs/open_file.h>
#include <kernel/vfs/path.h>
#include <memory/paging.h>
#include <stdint.h>
#include <tss.h>

#define KERNEL_PID 0

#define TASK_MAX_TIME_SLICE 25

#define DEFAULT_KERNEL_STACK_SIZE (0x100000 * 10)
#define DEFAULT_USER_STACK_SIZE (0x100000 * 10)

#define DEFAULT_USER_STACK_ADDRESS (USERLAND_MEMORY_BARRIER - DEFAULT_USER_STACK_SIZE)

#define WAIT_FOR_ANY_PROCESS -1

namespace influx {
namespace threading {
struct priority_tcb_queue {
    tcb *start;
    tcb *next_task;
};

void new_kernel_thread_wrapper(void (*func)(void *), void *data);
void new_user_process_wrapper(executable *exec);

class scheduler {
   public:
    scheduler(uint64_t tss_addr);

    bool started() const;

    tcb *create_kernel_thread(void (*func)(), void *data = nullptr, bool blocked = false,
                              uint64_t pid = KERNEL_PID);
    tcb *create_kernel_thread(void (*func)(void *), void *data, bool blocked = false,
                              uint64_t pid = KERNEL_PID);

    void sleep(uint64_t ms);
    int64_t wait_for_child(int64_t child_pid);

    void kill_current_task();

    void block_task(tcb *task);
    void block_current_task();
    void unblock_task(tcb *task);

    uint64_t exec(size_t fd, const structures::string &name,
                  const structures::vector<structures::string> &args,
                  const structures::vector<structures::string> &env);
    uint64_t sbrk(int64_t inc);

    uint64_t get_current_task_id() const;
    uint64_t get_current_process_id() const;

    uint64_t add_file_descriptor(const vfs::open_file &file);
    vfs::error get_file_descriptor(uint64_t fd, vfs::open_file &file);
    void update_file_descriptor(uint64_t fd, vfs::open_file &file);
    void remove_file_descriptor(uint64_t fd);

   private:
    logger _log;

    bool _started;

    structures::unique_hash_map<process> _processes;
    structures::vector<priority_tcb_queue> _priority_queues;
    structures::vector<tcb *> _killed_tasks_queue;

    tcb *_tasks_clean_task;
    tcb *_idle_task;
    tcb *_current_task;

    uint64_t _max_quantum;

    tss_t *_tss;

    init_process _init_process;

    tcb *get_next_task();
    tcb *update_priority_queue_next_task(uint16_t priority);

    void reschedule();
    void tick_handler();
    void update_tasks_sleep_quantum();
    void queue_task(tcb *task);

    void tasks_clean_task();
    void idle_task();

    uint64_t start_process(executable &exec);

    tcb *get_current_task() const;
    uint64_t get_stack_pointer() const;

    uint64_t pages_for_argv_envp(executable &exec);
    structures::pair<const char **, const char **> copy_argv_envp(executable &exec,
                                                                  uint64_t address);

    friend void new_user_process_wrapper(executable *exec);

    friend class mutex;
    friend class condition_variable;
    friend class irq_notifier;
    friend class init_process;
};
};  // namespace threading
};  // namespace influx