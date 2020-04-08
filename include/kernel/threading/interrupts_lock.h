#pragma once

namespace influx {
namespace threading {
class interrupts_lock {
   public:
    interrupts_lock(bool lock = true);
    ~interrupts_lock();

    void lock();
    void unlock();

   private:
    bool _locked;
};
};  // namespace threading
};  // namespace influx