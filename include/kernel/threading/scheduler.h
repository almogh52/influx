#pragma once
#include <kernel/logger.h>
#include <kernel/structures/unique_hash_map.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/process.h>
#include <kernel/threading/thread.h>

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

    const thread &create_kernel_thread(void (*func)(), void *data = nullptr);
    const thread &create_kernel_thread(void (*func)(void *), void *data);

    void kill_current_task();

   private:
    logger _log;

    structures::unique_hash_map<process> _processes;
    structures::vector<priority_tcb_queue> _priority_queues;
    structures::vector<tcb *> _killed_tasks_queue;

    tcb *_current_task;

    uint64_t _max_quantum;

    tcb *get_next_task();
    tcb *update_priority_queue_next_task(uint16_t priority);

    void reschedule();
    void tick_handler();

    void tasks_clean_task();

    void queue_task(tcb *task);

    uint64_t get_stack_pointer() const;
};
};  // namespace threading
};  // namespace influx