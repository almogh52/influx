#pragma once
#include <kernel/threading/mutex.h>
#include <kernel/threading/task_wait_queue.h>

namespace influx {
namespace threading {
class irq_notifier {
   public:
    irq_notifier();
    irq_notifier(const irq_notifier &) = delete;

    void notify() noexcept;
    void wait();
    bool wait_interruptible();

   private:
    bool _notified;
    tcb *_task;
};
};  // namespace threading
};  // namespace influx