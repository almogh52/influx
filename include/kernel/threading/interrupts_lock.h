#pragma once

namespace influx {
namespace threading {
class interrupts_lock {
   public:
    interrupts_lock();
    ~interrupts_lock();
};
};  // namespace threading
};  // namespace influx