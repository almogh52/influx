#pragma once
#include <kernel/threading/process.h>
#include <kernel/threading/thread.h>

namespace influx {
namespace threading {
namespace scheduler_utils {
extern "C" {
void switch_task(thread *current_task, thread *new_task, process *new_task_process);
uint64_t get_cr3();
};
};  // namespace scheduler_utils
};  // namespace threading
};  // namespace influx