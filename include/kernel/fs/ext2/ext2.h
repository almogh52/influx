#pragma once
#include <kernel/vfs/filesystem.h>

#include <kernel/fs/ext2/block_group_desc.h>
#include <kernel/fs/ext2/dir_entry.h>
#include <kernel/fs/ext2/inode.h>
#include <kernel/fs/ext2/superblock.h>
#include <kernel/structures/bitmap.h>
#include <kernel/structures/dynamic_buffer.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/mutex.h>

#define EXT2_MAGIC 0xEF53
#define EXT2_SUPERBLOCK_OFFSET 1024

#define EXT2_GOOD_OLD_REV 0  // The good old (original) format
#define EXT2_DYNAMIC_REV 1   // V2 format with dynamic inode sizes

#define EXT2_BGDT_BLOCK 1  // Block group descriptor table block

// Special inode numbers
#define EXT2_BAD_INO 1          // Bad blocks inode
#define EXT2_ROOT_INO 2         // Root inode
#define EXT2_BOOT_LOADER_INO 5  // Boot loader inode
#define EXT2_UNDEL_DIR_INO 6    // Undelete directory inode

#define EXT2_INVALID_BLOCK 0  // Superblock is always taken so use it as invalid file block
#define EXT2_INVALID_INODE 0  // inodes starts at 1

namespace influx {
namespace fs {
class ext2 : public vfs::filesystem {
   public:
    ext2(const drivers::ata::drive_slice& drive);

    virtual bool mount();
    inline virtual size_t read(void* fs_file_info, char* buffer, size_t count, size_t offset,
                               size_t& amount_read){};
    inline virtual size_t write(void* fs_file_info, const char* buffer, size_t count, size_t offset,
                                size_t& amount_written){};
    inline virtual size_t get_file_info(void* fs_file_info, vfs::file& file){};
    inline virtual size_t entries(void* fs_file_info,
                                  structures::vector<vfs::dir_entry>& entries){};
    inline virtual size_t create_file(const vfs::path& file_path){};
    inline virtual size_t create_dir(const vfs::path& dir_path){};
    inline virtual size_t remove(void* fs_file_info){};
    inline virtual void* get_fs_file_info(const vfs::path& file_path){};

   private:
    ext2_superblock _sb;
    threading::mutex _sb_mutex;  // Use this only when accessing non-const fields

    uint64_t _block_size;

    structures::vector<ext2_block_group_desc> _block_groups;
    structures::vector<threading::mutex> _block_groups_mutexes;

    structures::dynamic_buffer read_block(uint64_t block, uint64_t offset = 0, int64_t amount = -1);
    bool write_block(uint64_t block, structures::dynamic_buffer& buf, uint64_t offset = 0);

    ext2_inode* get_inode(uint64_t inode);
    uint64_t find_inode(const vfs::path& file_path);

    uint64_t get_block_for_offset(ext2_inode* inode, uint64_t offset);
    structures::dynamic_buffer read_file(ext2_inode* inode, uint64_t offset, uint64_t amount);

    uint64_t alloc_block();
    uint64_t alloc_inode();
    bool free_block(uint64_t block);
    bool free_inode(uint64_t inode);

    structures::vector<vfs::dir_entry> read_dir(ext2_inode* dir_inode);
    vfs::file_type file_type_for_dir_entry(ext2_dir_entry* dir_entry);
    vfs::file_type file_type_for_inode(ext2_inode* inode);

    bool save_block_group(uint64_t block_group);
};
};  // namespace fs
};  // namespace influx