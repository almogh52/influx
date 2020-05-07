#pragma once
#include <kernel/structures/hash_map.h>
#include <kernel/structures/node.h>
#include <stdint.h>

namespace influx {
namespace structures {
template <typename T>
class unique_hash_map : public hash_map<uint64_t, T> {
   public:
    inline unique_hash_map(T empty_item = T())
        : hash_map<uint64_t, T>(empty_item), _counter(0), _deleted_keys_list(nullptr) {}
    inline unique_hash_map(memblock::size_type bucket_count, T empty_item = T())
        : hash_map<uint64_t, T>(bucket_count, empty_item),
          _counter(0),
          _deleted_keys_list(nullptr) {}

    inline int64_t insert_unique(T value) {
        uint64_t key;
        node<uint64_t>* old_node;

        if (_deleted_keys_list == nullptr) {
            key = _counter;
        } else {
            key = _deleted_keys_list->value();
        }

        if (hash_map<uint64_t, T>::insert(pair<const uint64_t, T>(key, value)).second == false) {
            return -1;
        }

        if (key == _counter) {
            _counter++;
        } else {
            old_node = _deleted_keys_list;
            _deleted_keys_list = _deleted_keys_list->remove(_deleted_keys_list);
            delete old_node;
        }

        return key;
    }

    template <typename... Args>
    inline pair<hash_map_iterator<uint64_t, T, hash<uint64_t>>, bool> emplace_unique(
        Args&&... args) {
        uint64_t key;
        node<uint64_t>* old_node;

        if (_deleted_keys_list == nullptr) {
            key = _counter;
            _counter++;
        } else {
            key = _deleted_keys_list->value();
            old_node = _deleted_keys_list;
            _deleted_keys_list = _deleted_keys_list->remove(_deleted_keys_list);
            delete old_node;
        }

        return hash_map<uint64_t, T>::insert(pair<const uint64_t, T>(key, T(args...)));
    }

    inline typename hash_map<uint64_t, T>::size_type erase(
        const typename hash_map<uint64_t, T>::key_type& key) {
        if (hash_map<uint64_t, T>::erase(key)) {
            if (_deleted_keys_list == nullptr) {
                _deleted_keys_list = new node<uint64_t>(key);
            } else {
                _deleted_keys_list = _deleted_keys_list->insert(new node<uint64_t>(key));
            }

            return 1;
        }

        return 0;
    }

   private:
    uint64_t _counter;

    node<uint64_t>* _deleted_keys_list;
};
};  // namespace structures
};  // namespace influx