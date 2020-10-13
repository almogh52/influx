#pragma once
#include <kernel/structures/node.h>
#include <kernel/structures/vector.h>
#include <stdint.h>

namespace influx {
namespace structures {
class unique_vector : public vector<uint64_t> {
   public:
    inline unique_vector() : _counter(0), _deleted_values_list(nullptr) {}

    inline uint64_t insert_unique() {
        uint64_t val;
        node<uint64_t>* old_node;

        if (_deleted_values_list == nullptr) {
            val = _counter;
            _counter++;
        } else {
            val = _deleted_values_list->value();
            old_node = _deleted_values_list;
            _deleted_values_list = _deleted_values_list->remove(_deleted_values_list);
            delete old_node;
        }

        vector::push_back(val);
        return val;
    }

    inline vector<uint64_t>::iterator erase(vector<uint64_t>::const_iterator pos) {
        uint64_t val = *pos;
        uint64_t old_size = size();

        vector<uint64_t>::iterator it = vector<uint64_t>::erase(pos);
        if (size() != old_size) {
            if (_deleted_values_list == nullptr) {
                _deleted_values_list = new node<uint64_t>(val);
            } else {
                _deleted_values_list = _deleted_values_list->insert(new node<uint64_t>(val));
            }
        }

        return it;
    }

   private:
    uint64_t _counter;

    node<uint64_t>* _deleted_values_list;
};
};  // namespace structures
};  // namespace influx