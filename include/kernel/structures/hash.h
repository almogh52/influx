#pragma once
#include <stddef.h>
#include <stdint.h>

namespace influx {
namespace structures {
template <typename T>
struct hash {
    size_t operator()(const T&) const;
};

template <>
struct hash<uint8_t> {
    size_t operator()(const uint8_t& n) { return n; }
};

template <>
struct hash<int8_t> {
    size_t operator()(const int8_t& n) { return n; }
};

template <>
struct hash<uint16_t> {
    size_t operator()(const uint16_t& n) { return n; }
};

template <>
struct hash<int16_t> {
    size_t operator()(const int16_t& n) { return n; }
};

template <>
struct hash<uint32_t> {
    size_t operator()(const uint32_t& n) { return n; }
};

template <>
struct hash<int32_t> {
    size_t operator()(const int32_t& n) { return n; }
};

template <>
struct hash<uint64_t> {
    size_t operator()(const uint64_t& n) { return n; }
};

template <>
struct hash<int64_t> {
    size_t operator()(const int64_t& n) { return n; }
};
};  // namespace structures
};  // namespace influx