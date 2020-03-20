#pragma once
#include <stdint.h>

namespace influx {
namespace fs {
enum class ext2_state : uint16_t { valid = 0x1, error = 0x2 };

enum class ext2_error_behaviour : uint16_t { ignore = 0x1, remount_read_only = 0x2, panic = 0x3 };

struct ext2_superblock {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t reserved_blocks_count;  // Blocks reserved for superuser
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t superblock_block;
    uint32_t log_block_size;  // Shift 1024 to left by log block size to obtain block size
    uint32_t log_frag_size;   // Shift 1024 to left by log frag size to obtain fragment size
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t mount_times;      // Since last consistency check (fsck in linux)
    uint16_t max_mount_times;  // Max mounts before consistency check
    uint16_t magic;
    ext2_state state;
    ext2_error_behaviour errors_behaviour;
    uint16_t minor_revision_level;
    uint32_t last_check;      // Last consistency check
    uint32_t check_interval;  // Interval between forced consistency checks
    uint32_t os_id;
    uint32_t major_revision_level;
    uint16_t default_reserved_user_id;   // User ID that can use reserved blocks
    uint16_t default_reserved_group_id;  // Group ID that can use reserved blocks
};
};  // namespace fs
};  // namespace influx