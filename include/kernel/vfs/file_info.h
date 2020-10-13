#pragma once
#include <kernel/vfs/file_permissions.h>
#include <kernel/vfs/file_type.h>

namespace influx {
namespace vfs {
struct file_info {
    uint64_t inode;
    file_type type;
    uint64_t size;
    uint64_t blocks;  // 512 bytes blocks
    file_permissions permissions;
    uint64_t owner_user_id;
    uint64_t owner_group_id;
    uint64_t fs_block_size;
    uint64_t hard_links_count;
    uint64_t created;
    uint64_t modified;
    uint64_t accessed;
};
};  // namespace vfs
};  // namespace influx