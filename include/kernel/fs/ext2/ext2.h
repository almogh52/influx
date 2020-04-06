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
#include <kernel/vfs/file_permissions.h>
#include <kernel/vfs/file_type.h>

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

    virtual bool mount(const vfs::path& mount_path);
    virtual vfs::error read(void* fs_file_info, char* buffer, size_t count, size_t offset,
                            size_t& amount_read);
    virtual vfs::error write(void* fs_file_info, const char* buffer, size_t count, size_t offset,
                             size_t& amount_written);
    virtual vfs::error get_file_info(void* fs_file_info, vfs::file_info& file);
    virtual vfs::error entries(void* fs_file_info, structures::vector<vfs::dir_entry>& entries);
    virtual vfs::error create_file(const vfs::path& file_path, vfs::file_permissions permissions,
                                   void** fs_file_info_ptr);
    inline virtual vfs::error create_dir(const vfs::path& dir_path,
                                         vfs::file_permissions permissions,
                                         void** fs_file_info_ptr) {
        return vfs::error::success;
    }
    inline virtual vfs::error remove(void* fs_file_info) { return vfs::error::success; };
    virtual void* get_fs_file_data(const vfs::path& file_path);
    virtual bool compare_fs_file_data(void* fs_file_data_1, void* fs_file_data_2);

   private:
    ext2_superblock _sb;
    threading::mutex _sb_mutex;  // Use this only when accessing non-const fields

    uint32_t _block_size;

    structures::vector<ext2_block_group_desc> _block_groups;
    structures::vector<threading::mutex> _block_groups_mutexes;

    structures::dynamic_buffer read_block(uint32_t block, uint64_t offset = 0, int64_t amount = -1);
    bool write_block(uint32_t block, structures::dynamic_buffer& buf, uint64_t offset = 0);
    bool clear_block(uint32_t block);

    ext2_inode* get_inode(uint32_t inode);
    uint32_t find_inode(const vfs::path& file_path);

    uint32_t get_block_for_offset(ext2_inode* inode, uint64_t offset, bool allocate);
    structures::dynamic_buffer read_file(ext2_inode* inode, uint64_t offset, uint64_t amount);
    uint64_t write_file(ext2_inode* inode, uint64_t offset, structures::dynamic_buffer buf);

    uint32_t create_inode(vfs::file_type type, vfs::file_permissions permissions);
    influx::vfs::error create_dir_entry(ext2_inode* dir_inode, uint32_t file_inode,
                                        ext2_dir_entry_type entry_type,
                                        structures::string file_name);

    uint32_t alloc_block();
    uint32_t alloc_inode();
    bool free_block(uint32_t block);
    bool free_inode(uint32_t inode);

    structures::vector<vfs::dir_entry> read_dir(ext2_inode* dir_inode);
    vfs::file_type file_type_for_dir_entry(ext2_dir_entry* dir_entry);
    vfs::file_type file_type_for_inode(ext2_inode* inode);
    vfs::file_permissions file_permissions_for_inode(ext2_inode* inode);

    ext2_types_permissions ext2_file_types_permissions(vfs::file_type type,
                                                       vfs::file_permissions permissions);
    ext2_dir_entry_type ext2_dir_entry_type_for_file_type(vfs::file_type type);

    bool save_block_group(uint32_t block_group);
    bool save_inode(uint32_t inode, ext2_inode* inode_obj);
};
};  // namespace fs
};  // namespace influx