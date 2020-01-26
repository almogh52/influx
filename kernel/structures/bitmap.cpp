#include <kernel/structures/bitmap.h>

influx::structures::bit::bit(uint64_t *node, uint64_t node_index)
    : _node(node), _node_index(node_index) {}

bool influx::structures::bit::get() const {
    return (bool)((*_node & ((uint64_t)1 << (BITS_PER_NODE - _node_index - 1))) >>
                  (BITS_PER_NODE - _node_index - 1));
}

void influx::structures::bit::set(bool value) {
    // If the bit is set
    if (value) {
        *_node |= ((uint64_t)1 << (BITS_PER_NODE - _node_index - 1));
    } else {
        *_node &= ~((uint64_t)1 << (BITS_PER_NODE - _node_index - 1));
    }
}

influx::structures::bit &influx::structures::bit::operator=(const influx::structures::bit &other) {
    // If not a self-assignment
    if (this != &other) {
        set(other.get());
    }

    return *this;
}

influx::structures::bit &influx::structures::bit::operator=(const bool &value) {
    // Set the new value
    set(value);

    return *this;
}

influx::structures::bit::operator bool() const { return get(); }

influx::structures::bitmap::bitmap() : _bitmap(nullptr), _size(0), _next_search_node(0) {}

influx::structures::bitmap::bitmap(void *bitmap, uint64_t size)
    : _bitmap((uint64_t *)bitmap), _size(size), _next_search_node(0) {}

bool influx::structures::bitmap::get(uint64_t index) const {
    return index >= _size ? false : get_bit_for_index(index);
}

influx::structures::bit influx::structures::bitmap::get(uint64_t index) {
    return index >= _size ? bit(nullptr, 0) : get_bit_for_index(index);
}

void influx::structures::bitmap::set(uint64_t index, bool value) {
    bit b = get_bit_for_index(index);

    // If the index is valid
    if (index < _size) {
        // Set the value for the bit
        b = value;
    }
}

void influx::structures::bitmap::set(uint64_t start, uint64_t size, bool value) {
    uint64_t *start_node = _bitmap + start / BITS_PER_NODE;
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
        for (uint8_t i = 0; i < BITS_PER_NODE - (start % BITS_PER_NODE) && total_set < size; i++) {
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
            *(start_node + i) = 0xffffffffffffffff;
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

influx::structures::bit influx::structures::bitmap::operator[](uint64_t index) {
    return get(index);
}

uint64_t influx::structures::bitmap::size() { return _size; }

void *influx::structures::bitmap::get_raw() const { return (void *)_bitmap; }

bool influx::structures::bitmap::search(uint64_t batch_size, bool value, uint64_t &batch_index) {
    bool found =
        search(_next_search_node, _size / BITS_PER_NODE, batch_size, value, batch_index) ||
        (_next_search_node != 0 && search(0, _next_search_node, batch_size, value, batch_index));

    // If the batch was found, set the next search node
    if (found) {
        _next_search_node = (batch_index / BITS_PER_NODE) + (batch_size / BITS_PER_NODE);
    }

    return found;
}

uint64_t influx::structures::bitmap::search_bit(bool value, uint64_t &bit_index) {
    bool found = search_bit(_next_search_node * BITS_PER_NODE, _size, value, bit_index) ||
                 (_next_search_node != 0 &&
                  search_bit(0, _next_search_node * BITS_PER_NODE, value, bit_index));

    // If the bit was found, set the next search node
    if (found) {
        _next_search_node = bit_index / BITS_PER_NODE;
    }

    return found;
}

bool influx::structures::bitmap::search(uint64_t start_node, uint64_t end_node, uint64_t batch_size,
                                        bool value, uint64_t &batch_index) {
    uint64_t current_batch_start = start_node;
    uint64_t current_batch_size = 0;
    uint64_t current_node = start_node;

    uint64_t node_value = value ? 0xffffffffffffffff : 0x0;

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

            // If we found the wanted bits to complete the batch, add it to the current batch size
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

bool influx::structures::bitmap::search_bit(uint64_t start_index, uint64_t end_index, bool value,
                                            uint64_t &bit_index) {
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

influx::structures::bit influx::structures::bitmap::get_bit_for_index(uint64_t index) const {
    uint64_t node_index = index % BITS_PER_NODE;
    uint64_t *node = _bitmap + (index / BITS_PER_NODE);

    return bit(node, node_index);
}