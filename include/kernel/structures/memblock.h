#pragma once
#include <kernel/assert.h>
#include <stddef.h>
#include <stdint.h>

namespace influx {
namespace structures {
class memblock {
   public:
    typedef char value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type reference;
    typedef value_type const_reference;
    typedef size_t size_type;
    typedef const_pointer const_iterator;
    typedef pointer iterator;

    memblock(void);
    memblock(const void* p, size_type n);
    memblock(size_type n);
    memblock(const memblock& b);
    ~memblock(void);
    void assign(const memblock& b);
    void assign(const void* p, size_type n);
    void resize(size_type new_size);
    inline void clear(void) noexcept { resize(0); }
    inline size_type capacity(void) const { return _capacity; }
    inline size_type max_size(void) const { return _capacity; }
    inline bool empty(void) const { return _capacity == 0 || _data == nullptr; }
    inline pointer data(void) { return _data; }
    inline const_pointer data(void) const { return _data; }
    inline iterator begin(void) const { return iterator(data()); }
    inline iterator iat(size_type i) const {
        kassert(i <= capacity());
        return begin() + i;
    }
    inline iterator end(void) const { return iat(capacity()); }
    inline const memblock& operator=(const memblock& b) {
        assign(b);
        return *this;
    }

   private:
    size_type _capacity;
    pointer _data;
};
};  // namespace structures
};  // namespace influx