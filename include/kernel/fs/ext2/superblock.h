#pragma once
#include <stdint.h>

namespace influx {
namespace fs {
enum class ext2_state : uint16_t { valid = 0x1, error = 0x2 };

enum class ext2_error_behaviour : uint16_t { ignore = 0x1, remount_read_only = 0x2, panic = 0x3 };

struct ext2_superblock {
    uint32_t inodes_count;           // Total number of inodes in file system
    uint32_t blocks_count;           // Total number of blocks in file system
    uint32_t reserved_blocks_count;  // Number of blocks reserved for superuser
    uint32_t free_blocks_count;      // Total number of unallocated blocks
    uint32_t free_inodes_count;      // Total number of unallocated inodes
    uint32_t superblock_block;       // The block that contains the superblock
    uint32_t log_block_size;         // Shift 1024 to left by log block size to obtain block size
    uint32_t log_frag_size;          // Shift 1024 to left by log frag size to obtain fragment size
    uint32_t blocks_per_group;       // Number of blocks in each block group
    uint32_t frags_per_group;        // Number of fragments in each block group
    uint32_t inodes_per_group;       // Number of inodes in each block group
    uint32_t mount_time;             // Last mount time (in POSIX time)
    uint32_t write_time;             // Last written time (in POSIX time)
    uint16_t mount_times;            // Since last consistency check (fsck in linux)
    uint16_t max_mount_times;        // Max mounts before consistency check
    uint16_t magic;                  // Ext2 signature (0xef53)
    ext2_state state;                // File system state
    ext2_error_behaviour errors_behaviour;  // 	What to do when an error is detected
    uint16_t minor_revision_level;          // Minor portion of version
    uint32_t last_check;                    // Last consistency check
    uint32_t check_interval;                // Interval between forced consistency checks
    uint32_t os_id;  // Operating system ID from which the filesystem on this volume was created
    uint32_t major_revision_level;       // Major portion of version
    uint16_t default_reserved_user_id;   // User ID that can use reserved blocks
    uint16_t default_reserved_group_id;  // Group ID that can use reserved blocks
    uint32_t first_inode;                // First non-reserved inode in file system
    uint16_t inode_size;                 // Size of each inode structure in bytes
    uint16_t superblock_block_group;     // Block group that this superblock is part of
    uint32_t optional_features;  // Features that are not required to read or write, but usually
                                 // result in a performance increase.
    uint32_t required_features;  // Features that are required to be supported to read or write.
    uint32_t read_only_required_features;  // Features that if not supported, the volume must be
                                           // mounted read-only
    uint8_t uuid[16];                      // 128-bit uuid for volume
    char volume_name[16];        // Volume name (C-style string: characters terminated by a 0 byte)
    char last_mounted_path[64];  // Path volume was last mounted to (C-style string: characters
                                 // terminated by a 0 byte)
};
};  // namespace fs
};  // namespace influx