#pragma once
#include <kernel/structures/string.h>
#include <kernel/vfs/file_type.h>

namespace influx {
namespace vfs {
struct file {
    uint64_t inode;
    structures::string file_name;
    file_type type;
    uint64_t size;
    uint64_t created;
    uint64_t modified;
    uint64_t accessed;
};
};  // namespace vfs
};  // namespace influx