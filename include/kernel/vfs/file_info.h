#pragma once
#include <kernel/vfs/file_permissions.h>
#include <kernel/vfs/file_type.h>

namespace influx {
namespace vfs {
struct file_info {
    file_type type;
    uint64_t size;
    file_permissions permissions;
    uint64_t created;
    uint64_t modified;
    uint64_t accessed;
};
};  // namespace vfs
};  // namespace influx