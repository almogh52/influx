#pragma once
#include <stddef.h>
#include <stdint.h>

namespace influx {
namespace structures {
class fifo {
   public:
    fifo(uint8_t *buffer, uint64_t max_size);

    uint64_t current_size() const;

    bool push(const uint8_t *buffer, size_t count);
    bool pop(uint8_t *buffer, size_t count);

    inline uint8_t *buffer() const { return _buffer; }
    inline uint64_t max_size() const { return _max_size; }

   private:
    uint8_t *_buffer;
    uint64_t _max_size;

    uint64_t _head;
    uint64_t _tail;
};
};  // namespace structures
};  // namespace influx