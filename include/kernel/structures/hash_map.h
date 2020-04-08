#pragma once
#include <kernel/memory/utils.h>
#include <kernel/structures/hash.h>
#include <kernel/structures/memblock.h>
#include <kernel/structures/node.h>
#include <kernel/structures/pair.h>
#include <stddef.h>

#define DEFAULT_BUCKET_COUNT 16
#define DEFAULT_MAX_LOAD_FACTOR 1.0f

namespace influx {
namespace structures {
template <typename Key, typename T, typename Hash>
class hash_map;

template <typename Key, typename T, typename Hash>
struct hash_map_iterator {
   public:
    typedef Key key_type;
    typedef T mapped_type;
    typedef pair<const Key, T> value_type;
    typedef memblock::size_type size_type;

    inline hash_map_iterator(const hash_map_iterator<Key, T, Hash>& other)
        : _bucket_count(other._bucket_count),
          _buckets(other._buckets),
          _bucket_index(other._bucket_index),
          _element(other._element) {}

    inline hash_map_iterator<Key, T, Hash>& operator=(hash_map_iterator<Key, T, Hash> other) {
        _bucket_count = other._bucket_count;
        _buckets = other._buckets;
        _bucket_index = other._bucket_index;
        _element = other._element;

        return *this;
    }

    inline value_type& operator*() {
        kassert(_element != nullptr);

        return _element->value();
    }

    inline value_type* operator->() {
        kassert(_element != nullptr);

        return &_element->value();
    }

    inline hash_map_iterator<Key, T, Hash>& operator++() {
        // Move to the next element
        _element = _element->next();

        // If we reached the next element, search a new bucket
        if (_element == nullptr) {
            update_next_element(_bucket_index + 1);
        }

        return *this;
    }

    inline hash_map_iterator<Key, T, Hash>& operator++(int) { return ++(*this); }

    inline bool operator!=(hash_map_iterator<Key, T, Hash> other) {
        return _element != other._element;
    }
    inline bool operator==(hash_map_iterator<Key, T, Hash> other) {
        return _element == other._element;
    }

   private:
    typedef node<value_type> bucket_element_type;
    typedef bucket_element_type* bucket_type;

    size_type _bucket_count;
    memblock& _buckets;
    size_type _bucket_index;
    bucket_element_type* _element;

    inline hash_map_iterator(memblock& buckets)
        : _bucket_count(0), _buckets(buckets), _bucket_index(0), _element(nullptr) {}
    inline hash_map_iterator(uint64_t bucket_count, memblock& buckets, uint64_t bucket_index)
        : _bucket_count(bucket_count),
          _buckets(buckets),
          _bucket_index(bucket_index),
          _element(nullptr) {
        update_next_element(bucket_index);
    }
    inline hash_map_iterator(uint64_t bucket_count, memblock& buckets, uint64_t bucket_index,
                             bucket_element_type* element)
        : _bucket_count(bucket_count),
          _buckets(buckets),
          _bucket_index(bucket_index),
          _element(element) {}

    inline void update_next_element(uint64_t start_bucket_search) {
        for (size_type i = start_bucket_search; i < _bucket_count; i++) {
            if (*((bucket_type*)_buckets.data() + i) != nullptr) {
                _bucket_index = i;
                _element = *((bucket_type*)_buckets.data() + i);
                break;
            }
        }
    }

    friend class hash_map<Key, T, Hash>;
};

template <typename Key, typename T, typename Hash = hash<Key>>
class hash_map {
   public:
    typedef Key key_type;
    typedef T mapped_type;
    typedef pair<const Key, T> value_type;
    typedef memblock::size_type size_type;
    typedef Hash hasher;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef hash_map_iterator<Key, T, Hash> iterator;
    typedef hash_map_iterator<Key, T, Hash> const_iterator;

    inline hash_map(T empty_item = T()) : hash_map(DEFAULT_BUCKET_COUNT, empty_item) {}
    inline hash_map(size_type bucket_count, T empty_item = T())
        : _bucket_count(bucket_count),
          _buckets(_bucket_count * sizeof(bucket_type)),
          _size(0),
          _max_load_factor(DEFAULT_MAX_LOAD_FACTOR),
          _empty_item(empty_item) {
        memory::utils::memset(_buckets.data(), 0, _buckets.capacity());
    }

    inline iterator begin(void) { return iterator(_bucket_count, _buckets, 0); }
    inline const_iterator begin(void) const { return const_iterator(_bucket_count, _buckets, 0); }
    inline iterator end(void) { return iterator(_buckets); }
    inline const_iterator end(void) const { return const_iterator(_buckets); }
    inline const_iterator cbegin(void) const { return begin(); }
    inline const_iterator cend(void) const { return end(); }
    inline size_type size(void) const { return _size; }
    inline bool empty(void) const { return _size == 0; }
    inline size_type count(const key_type& key) const { return at(key) != _empty_item ? 1 : 0; };

    // Note: Non-standard function
    inline T& empty_item() { return _empty_item; };

    mapped_type& at(const key_type& key);
    const mapped_type& at(const key_type& key) const;

    mapped_type& operator[](const key_type& key);

    void clear();
    size_type erase(const key_type& key);
    inline pair<iterator, bool> insert(const value_type& value) {
        return insert(new bucket_element_type(value));
    }
    inline pair<iterator, bool> insert(value_type& value) {
        return insert((const value_type&)value);
    };

    inline void rehash(size_type count) {
        _bucket_count = count;
        rehash();
    };
    inline float load_factor() const { return _size / _bucket_count; };
    inline float max_load_factor() const { return _max_load_factor; };
    inline void max_load_factor(float l) { _max_load_factor = l; };
    inline size_type bucket_count() const { return _bucket_count; };
    inline size_type bucket(const Key& key) const { return Hash()(key) % _bucket_count; };

   private:
    typedef node<value_type> bucket_element_type;
    typedef bucket_element_type* bucket_type;

    size_type _bucket_count;
    memblock _buckets;
    size_type _size;
    float _max_load_factor;
    mapped_type _empty_item;

    void rehash();
    pair<iterator, bool> insert(bucket_element_type* element);
};
};  // namespace structures
};  // namespace influx

template <class Key, class T, class Hash>
T& influx::structures::hash_map<Key, T, Hash>::at(const key_type& key) {
    size_type bucket_index = bucket(key);
    bucket_type b = ((bucket_type*)_buckets.data())[bucket_index];

    // If the map is empty
    if (_size == 0) {
        return _empty_item;
    }

    // While the bucket pointer isn't null
    while (b != nullptr) {
        // If the wanted key was found, return it's value
        if (b->value().first == key) {
            return b->value().second;
        }

        // Set the next element as the current bucket pointer
        b = b->next();
    }

    return _empty_item;
}

template <class Key, class T, class Hash>
const T& influx::structures::hash_map<Key, T, Hash>::at(const key_type& key) const {
    size_type bucket_index = bucket(key);
    bucket_type b = ((bucket_type*)_buckets.data())[bucket_index];

    // If the map is empty
    if (_size == 0) {
        return _empty_item;
    }

    // While the bucket pointer isn't null
    while (b != nullptr) {
        // If the wanted key was found, return it's value
        if (b->value().first == key) {
            return b->value().second;
        }

        // Set the next element as the current bucket pointer
        b = b->next();
    }

    return _empty_item;
}

template <class Key, class T, class Hash>
T& influx::structures::hash_map<Key, T, Hash>::operator[](const key_type& key) {
    size_type bucket_index = bucket(key);
    bucket_type b = ((bucket_type*)_buckets.data())[bucket_index];

    // If the map is empty
    if (_size == 0) {
        return insert(pair<const Key, T>(key, _empty_item)).first->second;
    }

    // While the bucket pointer isn't null
    while (b != nullptr) {
        // If the wanted key was found, return it's value
        if (b->value().first == key) {
            return b->value().second;
        }

        // Set the next element as the current bucket pointer
        b = b->next();
    }

    return insert(pair<const Key, T>(key, _empty_item)).first->second;
}

template <class Key, class T, class Hash>
void influx::structures::hash_map<Key, T, Hash>::clear() {
    bucket_type b, temp;

    // If the hash map is empty
    if (_size == 0) {
        return;
    }

    // For each bucket, remove all of it's elements
    for (size_type i = 0; i < _bucket_count; i++) {
        // Get the bucket pointer
        b = ((bucket_type*)_buckets.data())[i];

        // While we didn't reach the end of the bucket
        while (b) {
            temp = b;

            // Set the next node
            b = b->next();

            // Free the current node
            delete b;
        }
    }
}

template <class Key, class T, class Hash>
influx::structures::memblock::size_type influx::structures::hash_map<Key, T, Hash>::erase(
    const key_type& key) {
    size_type bucket_index = bucket(key);
    bucket_type b = ((bucket_type*)_buckets.data())[bucket_index];

    // If the hash map is empty
    if (_size == 0) {
        return 0;
    }

    // While the bucket pointer isn't null
    while (b != nullptr) {
        // If the wanted key was found, erase it
        if (b->value().first == key) {
            // If it's the first element in the bucket, set the bucket as the next element
            if (b->prev() == nullptr) {
                ((bucket_type*)_buckets.data())[bucket_index] = b->next();
            } else if (b->prev() == nullptr && b->next()) {
                // Detach the node
                b->prev()->next() = b->next();
            }

            // Free the element
            delete b;

            // Decrease the size of the map
            _size -= 1;

            return 1;
        }

        // Set the next element as the current bucket pointer
        b = b->next();
    }

    return 0;
}

template <class Key, class T, class Hash>
influx::structures::pair<influx::structures::hash_map_iterator<Key, T, Hash>, bool>
influx::structures::hash_map<Key, T, Hash>::insert(bucket_element_type* element) {
    size_type bucket_index = bucket(element->value().first);
    bucket_type b = ((bucket_type*)_buckets.data())[bucket_index];

    bool inserted = false;
    bucket_type new_item = element;

    // If the first element is empty, insert the new value as the first element in the bucket
    if (b == nullptr) {
        inserted = true;
        new_item->next() = nullptr;
        new_item->prev() = nullptr;
        ((bucket_type*)_buckets.data())[bucket_index] = new_item;
    } else {
        // Get the element before the last element
        while (b->next() != nullptr && b->value().first != element->value().first) {
            b = b->next();
        }

        // If the key was found, don't create a new element
        if (b->value().first == element->value().first) {
            new_item = b;
            delete element;
        } else {
            inserted = true;
            b->next() = new_item;
            new_item->next() = nullptr;
            new_item->prev() = b;
        }
    }

    // Increase the size of the map if inserted
    if (inserted) {
        _size += 1;

        // If rehash is needed
        if ((float)_size > _max_load_factor * (float)_bucket_count) {
            rehash(_bucket_count * 2);
        }
    }

    return pair<iterator, bool>(iterator(_bucket_count, _buckets, bucket_index, new_item),
                                inserted);
}

template <class Key, class T, class Hash>
void influx::structures::hash_map<Key, T, Hash>::rehash() {
    memblock old_buckets(_buckets);

    size_type old_bucket_count = _buckets.capacity() / sizeof(bucket_type);
    bucket_type b = nullptr, prev = nullptr;

    // Create new memblock for the buckets
    _buckets.clear();
    _buckets.resize(_bucket_count * sizeof(bucket_type));
    memory::utils::memset(_buckets.data(), 0, _buckets.capacity());

    // Reset size of the map
    _size = 0;

    // Rehash all elements in old buckets
    for (size_type i = 0; i < old_bucket_count; i++) {
        b = ((bucket_type*)old_buckets.data())[i];

        // Insert every element in the bucket
        while (b) {
            // Move to the next element
            prev = b;
            b = b->next();

            // Insert the value
            insert(prev);
        }
    }
}