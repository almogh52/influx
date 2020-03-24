#pragma once
#include <stdint.h>

// Constants relative to the data blocks
#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS (EXT2_TIND_BLOCK + 1)

#define EXT2_INODE_TYPES_MASK 0xF000

namespace influx {
namespace fs {
enum class ext2_types_permissions : uint16_t {
    other_execute = 0x1,
    other_write = 0x2,
    other_read = 0x4,
    group_execute = 0x8,
    group_write = 0x10,
    group_read = 0x20,
    user_execute = 0x40,
    user_write = 0x80,
    user_read = 0x100,
    sticky_bit = 0x200,
    set_group_id = 0x400,
    set_user_id = 0x800,
    fifo = 0x1000,
    character_device = 0x2000,
    directory = 0x4000,
    block_device = 0x6000,
    regular_file = 0x8000,
    symbolic_link = 0xA000,
    unix_socket = 0xC000
};

inline bool operator&(ext2_types_permissions a, ext2_types_permissions b) {
    return static_cast<uint16_t>(a) & static_cast<uint16_t>(b);
}

enum class ext2_inode_flags : uint32_t {
    secure_deletion = 0x1,
    keep_copy = 0x2,
    file_compression = 0x4,
    synchronous_update = 0x8,
    immutable_file = 0x10,
    append_only = 0x20,
    dump_ignore = 0x40,
    no_last_access_update = 0x80,
    hash_indexed_directory = 0x10000,
    afs_directory = 0x20000,
    journal_file_data = 0x40000
};

struct ext2_inode {
    ext2_types_permissions types_permissions;
    uint16_t user_id;
    uint32_t size;
    uint32_t last_access_time;
    uint32_t creation_time;
    uint32_t last_modification_time;
    uint32_t deletion_time;
    uint16_t group_id;
    uint16_t hard_links_count;
    uint32_t disk_sectors_count;
    ext2_inode_flags flags;
    uint32_t os_specific_1;
    uint32_t block_pointers[EXT2_N_BLOCKS];
    uint32_t generation_number;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t fragment_address;
    uint32_t os_specific_2[3];
};
};  // namespace fs
};  // namespace influx