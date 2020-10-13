#pragma once
#include <kernel/threading/scheduler_started.h>

namespace influx {
namespace threading {
template <typename Mutex>
class lock_guard {
   public:
    lock_guard(Mutex& mutex) : _mutex(mutex) {
        if (scheduler_started) {
            _mutex.lock();
        }
    }

    lock_guard(const lock_guard& other) = delete;

    ~lock_guard() {
        if (scheduler_started) {
            _mutex.unlock();
        }
    }

   private:
    Mutex& _mutex;
};
};  // namespace threading
};  // namespace influx