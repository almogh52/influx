#pragma once
#include <kernel/memory/utils.h>
#include <kernel/structures/memblock.h>
#include <stddef.h>
#include <stdint.h>

namespace influx {
namespace structures {
template <typename T>
class vector {
   public:
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef pointer iterator;
    typedef const_pointer const_iterator;
    typedef memblock::size_type size_type;

    vector(void) {};
    explicit vector(size_type n);
    vector(const vector& v);
    vector(const_iterator i1, const_iterator i2);

    void resize(size_type n);
    inline const vector& operator=(const vector& v) {
        _data.assign(v._data);
        return *this;
    }
    inline size_type capacity(void) const { return _data.capacity() / sizeof(T); }
    inline size_type size(void) const { return _data.capacity() / sizeof(T); }
    inline size_type max_size(void) const { return _data.max_size() / sizeof(T); }
    inline bool empty(void) const { return _data.empty(); }
    inline iterator begin(void) { return iterator(_data.begin()); }
    inline const_iterator begin(void) const { return const_iterator(_data.begin()); }
    inline iterator end(void) { return iterator(_data.end()); }
    inline const_iterator end(void) const { return const_iterator(_data.end()); }
    inline const_iterator cbegin(void) const { return begin(); }
    inline const_iterator cend(void) const { return end(); }
    inline pointer data(void) { return pointer(_data.data()); }
    inline const_pointer data(void) const { return const_pointer(_data.data()); }
    inline iterator iat(size_type i) {
        kassert(i <= size());
        return begin() + i;
    }
    inline const_iterator iat(size_type i) const {
        kassert(i <= size());
        return begin() + i;
    }
    inline reference at(size_type i) {
        kassert(i < size());
        return begin()[i];
    }
    inline const_reference at(size_type i) const {
        kassert(i < size());
        return begin()[i];
    }
    inline reference operator[](size_type i) { return at(i); }
    inline const_reference operator[](size_type i) const { return at(i); }
    inline reference front(void) { return at(0); }
    inline const_reference front(void) const { return at(0); }
    inline reference back(void) {
        kassert(!empty());
        return end()[-1];
    }
    inline const_reference back(void) const {
        kassert(!empty());
        return end()[-1];
    }
    inline void clear(void) { _data.clear(); }
    void push_back(const T& v);
    void pop_back(void);
    inline const vector& operator+=(const T& v) { push_back(v); return *this; }

   private:
    memblock _data;
};  // namespace structures
};  // namespace structures
};  // namespace influx

template <typename T>
influx::structures::vector<T>::vector(influx::structures::memblock::size_type n) : _data() {
    resize(n);
}

template <typename T>
influx::structures::vector<T>::vector(const influx::structures::vector<T>& v) : _data(v._data) {}

template <typename T>
influx::structures::vector<T>::vector(const T* i1, const T* i2) {
    // Resize the vector
    resize(i2 - i1);

    // Copy elements
    memory::utils::memcpy(_data.data(), i1, (i2 - i1) * sizeof(T));
}

template <typename T>
void influx::structures::vector<T>::resize(influx::structures::memblock::size_type n) {
    size_type old_size = size();

    // Resize the vector
    _data.resize(n * sizeof(T));

    // If the vector got bigger
    if (old_size < n) {
        // Fill it with zeros
        memory::utils::memset(_data.data() + old_size * sizeof(T), 0, (n - old_size) * sizeof(T));
    }
}

template <typename T>
void influx::structures::vector<T>::push_back(const T& v) {
    size_type old_capacity = capacity();

    // Resize the vector for the new element
    resize(old_capacity + 1);

    // Set the new element
    at(old_capacity) = v;
}

template <typename T>
void influx::structures::vector<T>::pop_back() {
    kassert(capacity() != 0);

    // Resize the vector to remove the element
    resize(capacity() - 1);
}