#pragma once
#include <kernel/structures/memblock.h>
#include <stddef.h>
#include <stdint.h>

namespace influx {
namespace structures {
class string {
   public:
    typedef char value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef value_type const_reference;
    static constexpr const size_t npos = INT64_MAX;  ///< Value that means the end of string.
    static constexpr const size_t capacity_alignment = 32;

    string(const_pointer s, size_t len);
    string(size_t n, value_type c);
    inline string() : string("", 0) {}
    inline string(const_pointer s) : string(s, string::length(s)) {}
    inline string(const string& s) : string(s, s.length()) {}

    inline pointer data() { return _data.data(); }
    inline const_pointer data() const { return _data.data(); }
    inline const_pointer c_str() const { return _data.data(); }

    void resize(size_t n, value_type c);
    inline size_t capacity() const { return _data.capacity(); }
    inline void resize(size_t n) { resize(n, 0); }
    inline void clear() { _data.clear(); }

    reference at(size_t pos);
    inline const_reference at(size_t pos) const { return at(pos); }
    inline const_reference front() const { return at(0); }
    inline reference front() { return at(0); }
    inline const_reference back() const { return at(_length - 1); }
    inline reference back() { return at(_length - 1); }

    static size_t length(const_pointer s);
    inline size_t length() const { return _length; }
    inline size_t size() const { return _length; }

    string& append(const_pointer s, size_t len);
    string& append(size_t n, value_type c);
    inline string& append(const_pointer s) { return append(s, string::length(s)); }
    inline string& append(const string& s) { return append(s, s.length()); }

    string& assign(const_pointer s, size_t len);
    string& assign(size_t n, value_type c);
    inline string& assign(const_pointer s) { return assign(s, string::length(s)); }
    inline string& assign(const string& s) {
        _data = s._data;
        _length = s._length;
        return *this;
    }

    static int compare(const_pointer s1, const_pointer s2);
    inline int compare(const_pointer s) const { return string::compare(*this, s); }
    inline int compare(const string& s) const { return compare((const_pointer)s); }

    inline operator const_pointer() const { return _data.data(); }
    inline operator pointer() { return _data.data(); }

    string operator+(const string& s) const;
    inline string operator+(char c) const { return *this + string(1, c); }
    inline const string& operator=(const string& s) { return assign(s); }
    inline const string& operator=(const_pointer s) { return assign(s); }
    inline const string& operator=(const_reference c) { return assign(1, c); }
    inline const string& operator+=(const string& s) { return append(s); }
    inline const string& operator+=(const_pointer s) { return append(s); }
    inline const string& operator+=(value_type c) { return append(1, c); }
    inline bool operator==(const string& s) const { return compare(s) == 0; }
    inline bool operator==(const_pointer s) const { return compare(s) == 0; }
    inline bool operator==(value_type c) const { return _length == 1 && c == at(0); }
    inline bool operator!=(const string& s) const { return !operator==(s); }
    inline bool operator!=(const_pointer s) const { return !operator==(s); }
    inline bool operator!=(value_type c) const { return !operator==(c); }
    inline value_type& operator[](size_t pos) { return at(pos); }

    void reverse();

   private:
    memblock _data;
    size_t _length;

    inline static constexpr size_t capacity_for_string_size(size_t size) {
        return (size + 1) % capacity_alignment == 0
                   ? size + 1
                   : size + 1 + (capacity_alignment - ((size + 1) % capacity_alignment));
    }
};
};  // namespace structures
};  // namespace influx

inline influx::structures::string operator"" _s(const char* str, size_t len) {
    return influx::structures::string(str, len);
};
