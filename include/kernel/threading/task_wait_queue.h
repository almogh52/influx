#pragma once
#include <kernel/structures/node.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/spinlock.h>
#include <kernel/threading/thread.h>

namespace influx {
namespace threading {
class task_wait_queue {
   public:
    task_wait_queue();

    void enqueue(tcb *task);
    tcb *dequeue();
    structures::vector<tcb *> dequeue_all();

    bool empty();

   private:
    structures::node<tcb *> *_queue_head;
    spinlock _queue_lock;
};
};  // namespace threading
};  // namespace influx