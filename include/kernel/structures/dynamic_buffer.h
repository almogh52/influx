#pragma once
#include <kernel/structures/vector.h>

namespace influx {
namespace structures {
class dynamic_buffer : public vector<uint8_t> {
   public:
    inline dynamic_buffer() {}
    inline dynamic_buffer(vector<uint8_t>::size_type size) : vector<uint8_t>(size) {}
    inline dynamic_buffer(const dynamic_buffer& b) : vector<uint8_t>(b){};

    inline const dynamic_buffer& operator=(const dynamic_buffer& b) {
        vector<uint8_t>::operator=(b);
        return *this;
    }
};
};  // namespace structures
};  // namespace influx