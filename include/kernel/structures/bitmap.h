#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BITS_PER_NODE 64

namespace influx {
namespace structures {
class bit {
   public:
    bit(uint64_t* node, uint64_t node_index);

    bool get() const;
    void set(bool value);

    bit& operator=(const bit& other);
    bit& operator=(const bool& value);

    operator bool() const;

   private:
    uint64_t* _node;
    uint64_t _node_index;
};

class bitmap {
   public:
    bitmap();
    bitmap(void* bitmap, uint64_t size);

    bool get(uint64_t index) const;
    bit get(uint64_t index);

    void set(uint64_t index, bool value);
    void set(uint64_t start, uint64_t size, bool value);

    bool search(uint64_t batch_size, bool value, uint64_t& batch_index);
    uint64_t search_bit(bool value, uint64_t& batch_index);

    bit operator[](uint64_t index);

    uint64_t size();

    void* get_raw() const;

   private:
    uint64_t* _bitmap;
    uint64_t _size;

    uint64_t _next_search_node;

    bool search(uint64_t start_node, uint64_t end_node, uint64_t batch_size, bool value,
                uint64_t& batch_index);
    bool search_bit(uint64_t start_index, uint64_t end_index, bool value, uint64_t& batch_index);

    bit get_bit_for_index(uint64_t index) const;
};
};  // namespace structures
};  // namespace influx