#pragma once
#include <stdint.h>

namespace influx {
namespace fs {
struct ext2_block_group_desc {
    uint32_t block_bitmap_block;
    uint32_t inode_bitmap_block;
    uint32_t inode_table_block;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t dirs_count;
    uint16_t padding;
    uint32_t reserved[3];
};
};  // namespace fs
};  // namespace influx