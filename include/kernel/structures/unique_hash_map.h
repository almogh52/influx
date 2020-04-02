#pragma once
#include <kernel/structures/hash_map.h>
#include <stdint.h>

namespace influx {
namespace structures {
template <typename T>
class unique_hash_map : public hash_map<uint64_t, T> {
   public:
    inline unique_hash_map(T empty_item = T()) : hash_map<uint64_t, T>(empty_item), _counter(0) {}
    inline unique_hash_map(memblock::size_type bucket_count, T empty_item = T())
        : hash_map<uint64_t, T>(bucket_count, empty_item), _counter(0) {}

    inline int64_t insert_unique(T value) {
        if (hash_map<uint64_t, T>::insert(pair<const uint64_t, T>(_counter, value)).second ==
            false) {
            return -1;
        }

        return _counter++;
    }

    template <typename... Args>
    inline pair<hash_map_iterator<uint64_t, T, hash<uint64_t>>, bool> emplace_unique(
        Args&&... args) {
        return hash_map<uint64_t, T>::insert(pair<const uint64_t, T>(_counter, T(args...)));
    }

   private:
    uint64_t _counter;
};
};  // namespace structures
};  // namespace influx