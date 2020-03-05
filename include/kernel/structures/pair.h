#pragma once

namespace influx {
namespace structures {
template <typename T1, typename T2>
class pair {
   public:
    typedef T1 first_type;
    typedef T2 second_type;

    pair(){};
    pair(const T1& f, const T2& s) : first(f), second(s) {}
    pair(const pair& other) : first(other.first), second(other.second) {}

    pair& operator=(const pair& other) {
        first = other.first;
        second = other.second;

        return *this;
    }

    first_type first;
    second_type second;
};
};  // namespace structures
};  // namespace influx