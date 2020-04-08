#pragma once
#include <kernel/structures/string.h>
#include <kernel/vfs/file_type.h>
#include <stdint.h>

namespace influx {
namespace vfs {
struct dir_entry {
    uint64_t inode;
    file_type type;
    structures::string name;
};
};  // namespace vfs
};  // namespace influx