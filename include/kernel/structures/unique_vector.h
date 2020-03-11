#pragma once
#include <kernel/structures/vector.h>
#include <stdint.h>

namespace influx {
namespace structures {
class unique_vector : public vector<uint64_t> {
   public:
    inline uint64_t insert_unique() {
        vector::push_back(_counter);
        return _counter++;
    }

   private:
    uint64_t _counter;
};
};  // namespace structures
};  // namespace influx