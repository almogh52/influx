#pragma once
#include <kernel/fs/ext2/block_group_desc.h>
#include <kernel/fs/ext2/superblock.h>
#include <kernel/structures/vector.h>
#include <kernel/vfs/filesystem.h>

#define EXT2_MAGIC 0xEF53
#define EXT2_SUPERBLOCK_OFFSET 1024

#define EXT2_GOOD_OLD_REV 0 /* The good old (original) format */
#define EXT2_DYNAMIC_REV 1  /* V2 format w/ dynamic inode sizes */

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
    inline virtual size_t entries(void* fs_file_info, structures::vector<vfs::file>& contents){};
    inline virtual size_t create_file(const vfs::path& file_path){};
    inline virtual size_t create_dir(const vfs::path& dir_path){};
    inline virtual size_t remove(void* fs_file_info){};
    inline virtual void* get_fs_file_info(const vfs::path& file_path){};

   private:
    ext2_superblock _sb;
    uint64_t _block_size;
    structures::vector<ext2_block_group_desc> _block_groups;
};
};  // namespace fs
};  // namespace influx