#include <kernel/assert.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/utils.h>
#include <kernel/structures/string.h>

influx::structures::string::string(influx::structures::string::const_pointer s, size_t len)
    : _capacity(string::capacity_for_string_size(len)), _data((pointer)kcalloc(_capacity, sizeof(value_type))) {
    kassert(_data != nullptr);
    memory::utils::memcpy(_data, s, len);
}

influx::structures::string::string(size_t n, influx::structures::string::value_type c)
    : _capacity(string::capacity_for_string_size(n)), _data((pointer)kcalloc(_capacity, sizeof(value_type))) {
    kassert(_data != nullptr);
    memory::utils::memset(_data, c, n);
}

influx::structures::string::~string() {
    // If the data isn't null
    if (_capacity > 0 && _data) {
        kfree(_data);
    }
}

void influx::structures::string::resize(size_t n, influx::structures::string::value_type c) {
    pointer new_data = nullptr;

    size_t str_len = length();

    // If the current capacity isn't sufficient for the new string size
    if (n + 1 > _capacity) {
        // Allocate the new data
        _capacity = string::capacity_for_string_size(n);
        new_data = (pointer)kcalloc(_capacity, sizeof(value_type));

        // Copy old data
        memory::utils::memcpy(new_data, _data, str_len);

        // Free the old data
        if (_data) {
            kfree(_data);
        }

        // Set the new data as the data
        _data = new_data;
    }

    // If the new string length is bigger than the current length
    if (n > str_len) {
        // Extend the string and set it to the wanted character
        for (size_t i = str_len; i < n; i++) {
            _data[i] = c;
        }
    }

    // Set the end of the string
    _data[n] = 0;
}

influx::structures::string::reference influx::structures::string::at(size_t pos) {
    // Check for valid size
    kassert(pos < length());

    return _data[pos];
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
    size_t str_len = length();

    // Resize the string
    resize(str_len + len);

    // Copy the data from the other string
    memory::utils::memcpy(_data + str_len, s, len);

    return *this;
}

influx::structures::string &influx::structures::string::append(
    size_t n, influx::structures::string::value_type c) {
    size_t str_len = length();

    // Resize the string
    resize(str_len + n);

    // Set the character n times in the data
    memory::utils::memset(_data + str_len, c, n);

    return *this;
}

influx::structures::string &influx::structures::string::assign(
    influx::structures::string::const_pointer s, size_t len) {
    size_t new_capacity = string::capacity_for_string_size(len);

    // If the new capacity differs from the current capacity
    if (new_capacity != _capacity) {
        // Free the old data
        if (_data) {
            kfree(_data);
        }

        // Allocate the new data
        _capacity = new_capacity;
        _data = (pointer)kcalloc(new_capacity, sizeof(value_type));
    }

    // Copy the data from the string to the data
    memory::utils::memcpy(_data, s, len + 1);

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

void influx::structures::string::reverse()
{
    value_type temp;
    uint64_t str_len = length();

    // Swap matching characters in both ends of the string
    for (uint64_t i = 0; i < str_len / 2; i++) {
        temp = _data[i];
        _data[i] = _data[str_len - i - 1];
        _data[str_len - i - 1] = temp;
    }
}