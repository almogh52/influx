#include <kernel/structures/fifo.h>

influx::structures::fifo::fifo(uint8_t *buffer, uint64_t max_size)
    : _buffer(buffer), _max_size(max_size), _head(0), _tail(0) {}

uint64_t influx::structures::fifo::current_size() const { return _tail - _head; }

bool influx::structures::fifo::push(const uint8_t *buffer, size_t count) {
    // Verify the size of the FIFO after the insert
    if (current_size() + count > _max_size) {
        return false;
    }

    // Copy data from buffer to FIFO
    for (uint64_t i = _tail; i < (_tail + count); i++) {
        *(_buffer + (i % _max_size)) = *(buffer + (i - _tail));
    }

    // Increase tail ptr
    _tail += count;

    // Normalize the head and the tail to the range of the buffer
    if (_tail % _max_size > _head % _max_size) {
        _tail %= _max_size;
        _head %= _max_size;
    }

    return true;
}

bool influx::structures::fifo::pop(uint8_t *buffer, size_t count) {
    // Verify the FIFO has the amount of the wanted data
    if (current_size() < count) {
        return false;
    }

    // Copy data from FIFO to buffer
    for (uint64_t i = _head; i < (_head + count); i++) {
        *(buffer + (i - _head)) = *(_buffer + (i % _max_size));
    }

    // Increase head ptr
    _head += count;

    // Normalize the head and the tail to the range of the buffer
    if (_tail % _max_size > _head % _max_size) {
        _tail %= _max_size;
        _head %= _max_size;
    }

    return true;
}