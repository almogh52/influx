#pragma once
#include <kernel/structures/string.h>

namespace influx {
namespace vfs {
struct file {
    structures::string file_name;
    bool directory;
    bool system;
    uint64_t size;
    uint64_t created;
    uint64_t modified;
    uint64_t accessed;
};
};  // namespace vfs
};  // namespace influx