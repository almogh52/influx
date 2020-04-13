#pragma once
#include <kernel/threading/process.h>
#include <kernel/threading/thread.h>

namespace influx {
namespace threading {
namespace scheduler_utils {
extern "C" {
void switch_task(thread *current_task, thread *new_task, process *new_task_process);
void jump_to_ring_3(uint64_t ring_3_function_address, void *user_stack, uint64_t argc,
                    const char **argv, const char **envp);
};
};  // namespace scheduler_utils
};  // namespace threading
};  // namespace influx