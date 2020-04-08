#pragma once
#include <kernel/threading/mutex.h>
#include <kernel/threading/task_wait_queue.h>
#include <kernel/threading/unique_lock.h>

namespace influx {
namespace threading {
class condition_variable {
   public:
	condition_variable();
    condition_variable(const condition_variable&) = delete;

    void notify_one() noexcept;
	void notify_all() noexcept;

    void wait(unique_lock<mutex>& lock);

   private:
    task_wait_queue _wait_queue;
};
};  // namespace threading
};  // namespace influx