#include <kernel/structures/string.h>

#include <kernel/assert.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/utils.h>

influx::structures::string::string(influx::structures::string::const_pointer s, size_t len)
    : _data(string::capacity_for_string_size(len)), _length(len) {
    kassert(_data.data() != nullptr);

    // Copy string data
    memory::utils::memcpy(_data.data(), s, len);
    _data.data()[len] = (value_type)0;
}

influx::structures::string::string(size_t n, influx::structures::string::value_type c)
    : _data(string::capacity_for_string_size(n)), _length(n) {
    kassert(_data.data() != nullptr);

    // Set string data1
    memory::utils::memset(_data.data(), c, n);
    _data.data()[n] = (value_type)0;
}

void influx::structures::string::resize(size_t n, influx::structures::string::value_type c) {
    // If the current capacity isn't sufficient for the new string size
    if (n + 1 > _data.capacity()) {
        // Resize block size
        _data.resize(string::capacity_for_string_size(n));
    }

    // If the new string length is bigger than the current length
    if (n > _length) {
        // Extend the string and set it to the wanted character
        for (size_t i = _length; i < n; i++) {
            _data.data()[i] = c;
        }
    }

    // Set the end of the string
    _data.data()[n] = 0;

    // Set the new size of the string
    _length = n;
}

influx::structures::string::reference influx::structures::string::at(size_t pos) {
    // Check for valid size
    kassert(pos < _length);

    return (reference)*_data.iat(pos);
}

size_t influx::structures::string::length(const_pointer s) {
    size_t len = 0;

    // Check if the string pointer is null
    if (!s) {
        return 0;
    }

    // While we didn't reach the end of the string
    while (*(s + len)) {
        len++;
    }

    return len;
}

influx::structures::string &influx::structures::string::append(
    influx::structures::string::const_pointer s, size_t len) {
    size_t old_str_size = _length;

    // Resize the string
    resize(_length + len);

    // Copy the data from the other string
    memory::utils::memcpy(_data.data() + old_str_size, s, len);

    return *this;
}

influx::structures::string &influx::structures::string::append(
    size_t n, influx::structures::string::value_type c) {
    size_t old_str_size = _length;

    // Resize the string
    resize(_length + n);

    // Set the character n times in the data
    memory::utils::memset(_data.data() + old_str_size, c, n);

    return *this;
}

influx::structures::string &influx::structures::string::assign(
    influx::structures::string::const_pointer s, size_t len) {
    size_t new_capacity = string::capacity_for_string_size(len);

    // If the new capacity differs from the current capacity
    if (new_capacity != _data.capacity()) {
        _data.resize(new_capacity);
    }

    // Copy the data from the string to the data
    memory::utils::memcpy(_data.data(), s, len + 1);

    // Set the size of the string
    _length = len;

    return *this;
}

int influx::structures::string::compare(influx::structures::string::const_pointer s1,
                                        influx::structures::string::const_pointer s2) {
    // While the 2 strings didn't finish and the characters are matching
    while (*s1 && *s2 && *s1 == *s2) {
        ++s1;
        ++s2;
    }

    return (unsigned char)(*s1) - (unsigned char)(*s2);
}

influx::structures::string influx::structures::string::operator+(
    const influx::structures::string &s) const {
    string new_str(*this);

    // Add the other string to the new string
    new_str += s;

    return new_str;
}

void influx::structures::string::reverse() {
    value_type temp;

    // Swap matching characters in both ends of the string
    for (uint64_t i = 0; i < _length / 2; i++) {
        temp = _data.data()[i];
        _data.data()[i] = _data.data()[_length - i - 1];
        _data.data()[_length - i - 1] = temp;
    }
}