#pragma once
#include <stdint.h>

namespace influx {
class ports {
   public:
    template <typename T>
    static void out(T value, uint16_t port);

    template <typename T>
    static T in(uint16_t port);
};
};  // namespace influx