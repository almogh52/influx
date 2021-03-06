#pragma once
#include <kernel/threading/lock_types.h>
#include <kernel/threading/scheduler_started.h>

namespace influx {
namespace threading {
template <typename Mutex>
class unique_lock {
   public:
    unique_lock(Mutex& mutex) : _mutex(mutex), _locked(scheduler_started) {
        if (_locked) {
            _mutex.lock();
        }
    }

    unique_lock(Mutex& mutex, defer_lock_t t) : _mutex(mutex), _locked(false) {}
    unique_lock(Mutex& mutex, try_to_lock_t t)
        : _mutex(mutex), _locked(scheduler_started && mutex.try_lock()) {}
    unique_lock(Mutex& mutex, adopt_lock_t t) : _mutex(mutex), _locked(true) {}
    unique_lock(const unique_lock& other) = delete;

    ~unique_lock() {
        if (_locked) {
            _mutex.unlock();
        }
    }

    void lock() {
        if (scheduler_started && !_locked) {
            _mutex.lock();
            _locked = true;
        }
    }

    bool try_lock() {
        if (scheduler_started && !_locked) {
            _locked = _mutex.try_lock();
        }

        return _locked || !scheduler_started;
    }

    void unlock() {
        if (_locked) {
            _mutex.unlock();
            _locked = false;
        }
    }

    bool owns_lock() const { return _locked; }
    explicit operator bool() const { return _locked; }

   private:
    Mutex& _mutex;
    bool _locked;
};
};  // namespace threading
};  // namespace influx