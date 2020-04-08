#pragma once
#include <stdint.h>

namespace influx {
namespace threading {
class spinlock {
   public:
    spinlock();
    spinlock(const spinlock& other) = delete;

    spinlock& operator=(const spinlock& other) = delete;

    void lock();
    bool try_lock();

    void unlock();

   private:
    volatile uint32_t _value;
};
};  // namespace threading
};  // namespace influx