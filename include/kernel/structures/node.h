#pragma once

#include <stdint.h>

namespace influx {
namespace structures {
template <typename T>
class node {
   public:
    node(T val) : _value(val), _prev(nullptr), _next(nullptr) {}
    node(T val, node<T> *prev) : _value(val), _prev(prev), _next(nullptr) {}
    node(T val, node<T> *prev, node<T> *next) : _value(val), _prev(prev), _next(next) {}

    template <class... Args>
    node(Args... args) : _value(args...), _prev(nullptr), _next(nullptr) {}
    template <class... Args>
    node(Args... args, node<T> *prev) : _value(args...), _prev(prev), _next(nullptr) {}
    template <class... Args>
    node(Args... args, node<T> *prev, node<T> *next) : _value(args...), _prev(prev), _next(next) {}

    node<T> *insert(node<T> *new_node) {
        node<T> *current_node = this;

        // If the new node should replace the head
        if (new_node->_value < _value) {
            new_node->_next = this;
            _prev = new_node;

            return new_node;
        }

        // While we didn't reach the end of the list and the next node is bigger than the new
        // node
        while (current_node->_next != nullptr && current_node->_next->_value < new_node->_value) {
            // Move to the next node
            current_node = current_node->_next;
        }

        // Set the next and prev of the new node
        new_node->_next = current_node->_next;
        new_node->_prev = current_node;

        // Set the next node of the current node to the new node
        current_node->_next = new_node;

        // Set the next node's prev as the new node
        if (new_node->_next != nullptr) {
            new_node->_next->_prev = new_node;
        }

        return this;
    }

    void insert_next(node<T> *new_node) {
        // Set the next and prev of the new node
        new_node->_next = _next;
        new_node->_prev = this;

        // Set the next node of the current node to the new node
        _next = new_node;

        // Set the next node's prev as the new node
        if (new_node->_next != nullptr) {
            new_node->_next->_prev = new_node;
        }
    }

    node<T> *remove(node<T> *node) {
        // If the node to be deleted is the current node
        if (this == node) {
            if (_next != nullptr) {
                _next->_prev = _prev;
                return _next;
            } else {
                return nullptr;
            }
        } else {
            // If the node has a previous node, set the next of it to the next of the node
            if (node->_prev != nullptr) {
                node->_prev->_next = node->_next;
            }

            // If the node has a next node, set the prev of it to the prev of the node
            if (node->_next != nullptr) {
                node->_next->_prev = node->_prev;
            }
        }

        return this;
    }

    node<T> *find_node(T val) {
        node<T> *current_node = this;

        // While we didn't reach the end of the list
        while (current_node != nullptr) {
            // If the value of the node equals to the value of the node
            if (current_node->_value == val) {
                return current_node;
            } else {
                current_node = current_node->_next;
            }
        }

        return nullptr;
    }

    node<T> *find_node(T val, bool (*compare_func)(T &, T &)) {
        node<T> *current_node = this;

        // While we didn't reach the end of the list
        while (current_node != nullptr) {
            // If the value of the node equals to the value of the node
            if (compare_func(current_node->_value, val)) {
                return current_node;
            } else {
                current_node = current_node->_next;
            }
        }

        return nullptr;
    }

    template <typename C>
    node<T> *find_node(C val, bool (*compare_func)(T &, C &)) {
        node<T> *current_node = this;

        // While we didn't reach the end of the list
        while (current_node != nullptr) {
            // If the value of the node equals to the value of the node
            if (compare_func(current_node->_value, val)) {
                return current_node;
            } else {
                current_node = current_node->_next;
            }
        }

        return nullptr;
    }

    uint64_t length() {
        uint64_t len = 0;

        node<T> *current_node = this;

        // While we didn't reach the end of the list
        while (current_node != nullptr) {
            len++;
            current_node = current_node->_next;
        }

        return len;
    }

    T &value() { return _value; }

    node<T> *&prev() { return _prev; }

    node<T> *&next() { return _next; }

   private:
    T _value;

    node<T> *_prev, *_next;
};
};  // namespace structures
};  // namespace influx