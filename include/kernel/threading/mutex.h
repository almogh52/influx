#pragma once
#include <kernel/threading/spinlock.h>
#include <kernel/threading/task_wait_queue.h>
#include <stdint.h>

namespace influx {
namespace threading {
class mutex {
   public:
    mutex();
    mutex(const mutex& other) = delete;

    mutex& operator=(const mutex& other) = delete;

	void lock();
	bool try_lock();

	void unlock();

   private:
    uint32_t _value;
    spinlock _value_lock;

    task_wait_queue _wait_queue;
};
};  // namespace threading
};  // namespace influx