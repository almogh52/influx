#pragma once

namespace influx {
class algorithm {
   public:
    template <typename T>
    static T max(T a, T b) {
        return a > b ? a : b;
    };

    template <typename T>
    static T min(T a, T b) {
        return a > b ? b : a;
    };
};
};  // namespace influx