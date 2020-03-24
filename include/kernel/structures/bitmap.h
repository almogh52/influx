#pragma once

#include <stdbool.h>
#include <stdint.h>
#define BITS_PER_NODE (sizeof(Node) * 8)

namespace influx {
namespace structures {
template <typename Node>
class bit {
   public:
    inline bit(Node* node, uint64_t node_index, bool reversed)
        : _reversed(reversed), _node(node), _node_index(node_index) {}

    inline bool get() const {
        return (bool)((*_node & ((Node)1 << mask_shift_amount())) >> mask_shift_amount());
    }

    inline void set(bool value) {
        // If the bit is set
        if (value) {
            *_node = *_node | (Node)((Node)1 << mask_shift_amount());
        } else {
            *_node = *_node & (Node) ~((Node)1 << mask_shift_amount());
        }
    }

    inline bit& operator=(const bit& other) {
        // If not a self-assignment
        if (this != &other) {
            set(other.get());
        }

        return *this;
    }

    inline bit& operator=(const bool& value) {
        // Set the new value
        set(value);

        return *this;
    }

    inline operator bool() const { return get(); }

   private:
    bool _reversed;

    Node* _node;
    uint64_t _node_index;

    Node mask_shift_amount() const {
        return (Node)(_reversed ? (BITS_PER_NODE - _node_index - 1) : _node_index);
    }
};

template <typename Node = uint64_t>
class bitmap {
   public:
    inline bitmap(bool reversed = true)
        : _reversed(reversed), _bitmap(nullptr), _size(0), _next_search_node(0) {}

    inline bitmap(void* bitmap, uint64_t size, bool reversed = true)
        : _reversed(reversed), _bitmap((Node*)bitmap), _size(size), _next_search_node(0) {}

    inline bool get(uint64_t index) const {
        return index >= _size ? false : get_bit_for_index(index);
    }

    inline bit<Node> get(uint64_t index) {
        return index >= _size ? bit<Node>(nullptr, 0, _reversed) : get_bit_for_index(index);
    }

    inline void set(uint64_t index, bool value) {
        bit b = get_bit_for_index(index);

        // If the index is valid
        if (index < _size) {
            // Set the value for the bit
            b = value;
        }
    }

    inline void set(uint64_t start, uint64_t size, bool value) {
        uint64_t* start_node = _bitmap + start / BITS_PER_NODE;
        uint64_t amount_of_nodes =
            (size - (start % BITS_PER_NODE) - ((start + size) % BITS_PER_NODE)) / BITS_PER_NODE;

        uint64_t total_set = 0;

        // Check for valid start
        if (start > _size) {
            return;
        }

        // If the start is in a middle of a node
        if (start % BITS_PER_NODE) {
            // Set the bits in the first node
            for (uint8_t i = 0; i < BITS_PER_NODE - (start % BITS_PER_NODE) && total_set < size;
                 i++) {
                bit b = get_bit_for_index(start + i);
                b = value;

                // Add the amount of bits set
                total_set++;
            }

            // Skip the first node
            start_node++;
        }

        // Set all the full nodes
        for (uint64_t i = 0; i < amount_of_nodes && i < (_size / BITS_PER_NODE) && total_set < size;
             i++) {
            if (value) {
                *(start_node + i) = (Node)0xffffffffffffffff;
            } else {
                *(start_node + i) = 0;
            }

            // Add the amount of bits set
            total_set += BITS_PER_NODE;
        }

        for (uint8_t i = 0; total_set < size; i++) {
            bit b = get_bit_for_index(
                ((uint64_t)(start_node - _bitmap) + amount_of_nodes) * BITS_PER_NODE + i);
            b = value;

            // Add the amount of bits set
            total_set++;
        }
    }

    inline bool search(uint64_t batch_size, bool value, uint64_t& batch_index) {
        bool found =
            search(_next_search_node, _size / BITS_PER_NODE, batch_size, value, batch_index) ||
            (_next_search_node != 0 &&
             search(0, _next_search_node, batch_size, value, batch_index));

        // If the batch was found, set the next search node
        if (found) {
            _next_search_node = (batch_index / BITS_PER_NODE) + (batch_size / BITS_PER_NODE);
        }

        return found;
    }

    inline bool search_bit(bool value, uint64_t& bit_index) {
        bool found = search_bit(_next_search_node * BITS_PER_NODE, _size, value, bit_index) ||
                     (_next_search_node != 0 &&
                      search_bit(0, _next_search_node * BITS_PER_NODE, value, bit_index));

        // If the bit was found, set the next search node
        if (found) {
            _next_search_node = bit_index / BITS_PER_NODE;
        }

        return found;
    }

    inline bit<Node> operator[](uint64_t index) { return get(index); }

    inline uint64_t size() const { return _size; };

    inline void* get_raw() const { return (void*)_bitmap; }

   private:
    bool _reversed;

    Node* _bitmap;
    uint64_t _size;

    uint64_t _next_search_node;

    inline bool search(uint64_t start_node, uint64_t end_node, uint64_t batch_size, bool value,
                       uint64_t& batch_index) {
        uint64_t current_batch_start = start_node;
        uint64_t current_batch_size = 0;
        uint64_t current_node = start_node;

        Node node_value = value ? (Node)0xffffffffffffffff : 0x0;

        // While we didn't reach the wanted batch size
        while (current_batch_size != batch_size && current_node <= end_node) {
            // If the next search is a full node and the next node equals to the wanted node value
            if (batch_size - current_batch_size >= BITS_PER_NODE &&
                *(_bitmap + current_node) == node_value) {
                current_batch_size += BITS_PER_NODE;
            } else if (batch_size - current_batch_size < BITS_PER_NODE) {
                uint32_t i = 0;

                // Check each bit in the node
                for (i = 0; i < (uint32_t)(batch_size - current_batch_size); i++) {
                    // If the bit doesn't have the correct value, reset the current batch
                    if (get_bit_for_index(current_node * BITS_PER_NODE + i).get() != value) {
                        // Reset the current batch
                        current_batch_size = 0;
                        current_batch_start = current_node + 1;

                        break;
                    }
                }

                // If we found the wanted bits to complete the batch, add it to the current batch
                // size
                if (i == (uint32_t)(batch_size - current_batch_size)) {
                    current_batch_size += i;
                }
            } else {
                // Reset the current batch
                current_batch_size = 0;
                current_batch_start = current_node + 1;
            }

            // Move to the next node
            current_node++;
        }

        // If we found the batch, set in the return batch index
        if (current_batch_size == batch_size) {
            batch_index = current_batch_start * BITS_PER_NODE;
        }

        return current_batch_size == batch_size;
    }

    inline bool search_bit(uint64_t start_index, uint64_t end_index, bool value,
                           uint64_t& bit_index) {
        uint64_t current_index = start_index;

        // While we didn't reach the last index
        while (current_index < end_index) {
            if (get(current_index).get() == value) {
                break;
            } else {
                current_index++;
            }
        }

        // If we found a bit that matches the value
        if (current_index < end_index) {
            bit_index = current_index;

            return true;
        } else {
            return false;
        }
    }

    bit<Node> get_bit_for_index(uint64_t index) const {
        uint64_t node_index = index % BITS_PER_NODE;
        Node* node = _bitmap + (index / BITS_PER_NODE);

        return bit<Node>(node, node_index, _reversed);
    }
};
};  // namespace structures
};  // namespace influx