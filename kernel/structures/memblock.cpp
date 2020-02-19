#include <kernel/structures/memblock.h>

#include <kernel/memory/heap.h>
#include <kernel/memory/utils.h>

influx::structures::memblock::memblock() : _capacity(0),_data(nullptr) {}

influx::structures::memblock::memblock(const void *p, influx::structures::memblock::size_type n)
    : _capacity(n), _data((pointer)p) {}

influx::structures::memblock::memblock(influx::structures::memblock::size_type n)
    : memblock(kmalloc(n), n) {}

influx::structures::memblock::memblock(const influx::structures::memblock &b)
    : _capacity(b._capacity), _data((pointer)kmalloc(_capacity)) {
    // Copy the data
    memory::utils::memcpy(_data, b._data, _capacity);
}

influx::structures::memblock::~memblock() {
    // If the data pointer isn't null
    if (_data != nullptr) {
        kfree(_data);
    }
}

void influx::structures::memblock::assign(const void *p,
                                          influx::structures::memblock::size_type n) {
    _data = (pointer)p;
    _capacity = n;
}

void influx::structures::memblock::resize(influx::structures::memblock::size_type new_size) {
    _capacity = new_size;
    _data = (pointer)krealloc(_data, new_size);
}