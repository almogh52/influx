#pragma once
#include <kernel/logger.h>
#include <kernel/structures/unique_hash_map.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/process.h>
#include <kernel/threading/thread.h>
#include <kernel/vfs/error.h>
#include <kernel/vfs/open_file.h>

#define TASK_MAX_TIME_SLICE 25

#define DEFAULT_KERNEL_STACK_SIZE 0x800000

namespace influx {
namespace threading {
struct priority_tcb_queue {
    tcb *start;
    tcb *next_task;
};

void new_thread_wrapper(void (*func)(void *), void *data);

class scheduler {
   public:
    scheduler();

    bool started() const;

    tcb *create_kernel_thread(void (*func)(), void *data = nullptr, bool blocked = false);
    tcb *create_kernel_thread(void (*func)(void *), void *data, bool blocked = false);

    void sleep(uint64_t ms);
    void kill_current_task();

    void block_task(tcb *task);
    void block_current_task();
    void unblock_task(tcb *task);

    uint64_t get_current_task_id() const;
    uint64_t get_current_process_id() const;

    uint64_t add_file_descriptor(const vfs::open_file &file);
    vfs::error get_file_descriptor(uint64_t fd, vfs::open_file &file);
    void update_file_descriptor(uint64_t fd, vfs::open_file &file);

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

    tcb *get_next_task();
    tcb *update_priority_queue_next_task(uint16_t priority);

    void reschedule();
    void tick_handler();
    void update_tasks_sleep_quantum();

    void tasks_clean_task();
    void idle_task();

    void queue_task(tcb *task);

    tcb *get_current_task() const;

    uint64_t get_stack_pointer() const;

    friend class mutex;
    friend class condition_variable;
    friend class irq_notifier;
};
};  // namespace threading
};  // namespace influx