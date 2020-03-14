#pragma once

namespace influx {
namespace threading {
template <typename Mutex>
class lock_guard {
   public:
    lock_guard(Mutex& mutex) : _mutex(mutex) { _mutex.lock(); }
    lock_guard(const lock_guard& other) = delete;

    ~lock_guard() { _mutex.unlock(); }

   private:
    Mutex& _mutex;
};
};  // namespace threading
};  // namespace influx