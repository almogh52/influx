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

    template <class InputIterator, class T>
    static InputIterator find(InputIterator first, InputIterator last, const T& val) {
        while (first != last) {
            if (*first == val) return first;
            ++first;
        }
        return last;
    }
};
};  // namespace influx