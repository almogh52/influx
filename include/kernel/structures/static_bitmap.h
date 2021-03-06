#pragma once

#include <kernel/structures/bitmap.h>
#include <stdint.h>

namespace influx {
namespace structures {
template <uint64_t static_size, typename Node = uint64_t>
class static_bitmap : public bitmap<Node> {
   public:
    static_bitmap() : bitmap<Node>((void *)_bitmap, static_size){};

   private:
    uint64_t _bitmap[static_size / BITS_PER_NODE + (static_size % BITS_PER_NODE ? 1 : 0)];
};
};  // namespace structures
};  // namespace influx