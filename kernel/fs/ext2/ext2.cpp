#include <kernel/fs/ext2/ext2.h>

#include <kernel/algorithm.h>
#include <kernel/kernel.h>
#include <kernel/memory/utils.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/unique_lock.h>
#include <kernel/time/time_manager.h>

#define EXT2_IND_N_BLOCKS (_block_size / sizeof(uint32_t))
#define EXT2_DIND_N_BLOCKS (EXT2_IND_N_BLOCKS * EXT2_IND_N_BLOCKS)
#define EXT2_TIND_N_BLOCKS (EXT2_IND_N_BLOCKS * EXT2_DIND_N_BLOCKS)

influx::fs::ext2::ext2(const influx::drivers::ata::drive_slice &drive)
    : vfs::filesystem("EXT2", drive), _block_size(0) {}

bool influx::fs::ext2::mount(const influx::vfs::path &mount_path) {
    uint32_t number_of_groups = 0;

    structures::string mount_path_str = mount_path.string();

    // Try to read the superblock
    if (!_drive.read(EXT2_SUPERBLOCK_OFFSET, sizeof(ext2_superblock), &_sb)) {
        return false;
    }

    // If the magic number is incorrect
    if (_sb.magic != EXT2_MAGIC) {
        _log("EXT2 Magic not found!\n");
        return false;
    }
    _log("Valid EXT2 magic number found. EXT revision: %d\n", _sb.major_revision_level);
    _log("Superblock: inode count: %d, inodes per group: %d, blocks per group: %d.\n",
         _sb.inodes_count, _sb.inodes_per_group, _sb.blocks_per_group);

    // Calculate block size
    _block_size = (1024 << _sb.log_block_size);
    _log("Block size is %x bytes.\n", _block_size);

    // Calculate number of groups
    number_of_groups = _sb.blocks_count / _sb.blocks_per_group;
    if (_sb.blocks_per_group * number_of_groups < _sb.blocks_count) {
        number_of_groups++;
    }
    _log("Total of %d block groups.\n", number_of_groups);

    // Read the block group descriptor table
    _log("Reading block group descriptor table..\n");
    _block_groups.resize(number_of_groups);
    _block_groups_mutexes.resize(number_of_groups);
    if (!_drive.read(_block_size * EXT2_BGDT_BLOCK,
                     sizeof(ext2_block_group_desc) * number_of_groups, _block_groups.data())) {
        _log("Unable to read block group descriptor table from drive!\n");
        return false;
    }

    // Set last mount time and last mount path
    _log("Setting last mount time and last mount path..\n");
    _sb.mount_time = (uint32_t)kernel::time_manager()->unix_timestamp();
    memory::utils::memcpy(_sb.last_mounted_path, mount_path_str.c_str(),
                          algorithm::min<uint64_t>(mount_path_str.size() + 1, 64));
    if (!_drive.write(EXT2_SUPERBLOCK_OFFSET, sizeof(_sb), &_sb)) {
        return false;
    }

    return true;
}

influx::vfs::error influx::fs::ext2::read(void *fs_file_info, char *buffer, size_t count,
                                          size_t offset, size_t &amount_read) {
    kassert(fs_file_info != nullptr && buffer != nullptr && offset > 0 && count > 0);

    structures::dynamic_buffer buf;

    // Get the inode of the file
    ext2_inode *inode = get_inode(*(uint32_t *)fs_file_info);
    if (!inode) {
        return vfs::error::invalid_file;
    }

    // Check if the file is a directory
    if (file_type_for_inode(inode) == vfs::file_type::directory) {
        return vfs::error::file_is_directory;
    }

    // If the offset + the amount of bytes surpasses the size of the file, reduce the count
    if (offset + count > inode->size) {
        count = inode->size - offset;
    }

    // Try to read the file
    buf = read_file(inode, offset, count);
    if (buf.empty()) {
        return vfs::error::io_error;
    }

    // Set the amount read variable
    amount_read = buf.size();

    // Copy to the buffer
    memory::utils::memcpy(buffer, buf.data(), buf.size());

    // Save the inode
    if (!save_inode(*(uint32_t *)fs_file_info, inode)) {
        return vfs::error::io_error;
    }

    return vfs::error::success;
}

influx::vfs::error influx::fs::ext2::write(void *fs_file_info, const char *buffer, size_t count,
                                           size_t offset, size_t &amount_written) {
    kassert(fs_file_info != nullptr && buffer != nullptr && offset > 0 && count > 0);

    structures::dynamic_buffer buf(count);

    // Get the inode of the file
    ext2_inode *inode = get_inode(*(uint32_t *)fs_file_info);
    if (!inode) {
        return vfs::error::invalid_file;
    }

    // Check if the file is a directory
    if (file_type_for_inode(inode) == vfs::file_type::directory) {
        return vfs::error::file_is_directory;
    }

    // Copy to the buffer
    memory::utils::memcpy(buf.data(), buffer, buf.size());

    // Try to write to the file
    amount_written = write_file(inode, offset, buf);

    // Save the inode
    if (!save_inode(*(uint32_t *)fs_file_info, inode)) {
        return vfs::error::io_error;
    }

    return vfs::error::success;
}

influx::vfs::error influx::fs::ext2::get_file_info(void *fs_file_info,
                                                   influx::vfs::file_info &file) {
    kassert(fs_file_info != nullptr);

    // Get the inode of the file
    ext2_inode *inode = get_inode(*(uint32_t *)fs_file_info);
    if (!inode) {
        return vfs::error::invalid_file;
    }

    // Set the properties of the file
    file.type = file_type_for_inode(inode);
    file.size = inode->size;
    file.permissions = file_permissions_for_inode(inode);
    file.modified = inode->last_modification_time;
    file.modified = inode->last_access_time;
    file.created = inode->creation_time;

    return vfs::error::success;
}

influx::vfs::error influx::fs::ext2::entries(
    void *fs_file_info, influx::structures::vector<influx::vfs::dir_entry> &entries) {
    kassert(fs_file_info != nullptr);

    // Get the inode of the file
    ext2_inode *inode = get_inode(*(uint32_t *)fs_file_info);
    if (!inode) {
        return vfs::error::invalid_file;
    }

    // Check if the file is a directory
    if (file_type_for_inode(inode) == vfs::file_type::directory) {
        return vfs::error::file_is_not_directory;
    }

    // Set the properties of the file
    entries = read_dir(inode);

    // Save the inode of the dir
    if (!save_inode(*(uint32_t *)fs_file_info, inode)) {
        return vfs::error::io_error;
    }

    return vfs::error::success;
}

influx::vfs::error influx::fs::ext2::create_file(const influx::vfs::path &file_path,
                                                 influx::vfs::file_permissions permissions,
                                                 void **fs_file_info_ptr) {
    uint32_t dir_inode = find_inode(file_path.parent_path());
    ext2_inode *dir_inode_obj = nullptr;
    structures::vector<vfs::dir_entry> dir_entries;

    uint32_t file_inode = EXT2_INVALID_INODE;

    vfs::error err = vfs::error::success;

    // Base dir not found
    if (dir_inode == EXT2_INVALID_INODE) {
        return vfs::error::file_not_found;
    }

    // Get the inode of the dir
    dir_inode_obj = get_inode(dir_inode);
    if (dir_inode_obj == nullptr) {
        return vfs::error::io_error;
    }

    // Get dir entries and check that the name isn't taken
    dir_entries = read_dir(dir_inode_obj);
    for (const auto &dir_entry : dir_entries) {
        // If the name is taken
        if (dir_entry.name == file_path.base_name()) {
            return vfs::error::file_already_exists;
        }
    }

    // Create the new file
    file_inode = create_inode(vfs::file_type::regular, permissions);
    if (file_inode == EXT2_INVALID_INODE) {
        delete dir_inode_obj;
        return vfs::error::quota_exhausted;
    }

    // Create the dir entry for the file
    if ((err = create_dir_entry(dir_inode_obj, file_inode, ext2_dir_entry_type::regular,
                                file_path.base_name())) != vfs::error::success) {
        delete dir_inode_obj;
        free_inode(file_inode);
        return err;
    }

    // Free dir inode object
    delete dir_inode_obj;

    // Get the inode object
    *fs_file_info_ptr = get_inode(file_inode);
    if (*fs_file_info_ptr == nullptr) {
        return vfs::error::io_error;
    }

    return vfs::error::success;
}

void *influx::fs::ext2::get_fs_file_data(const influx::vfs::path &file_path) {
    uint32_t inode = find_inode(file_path);

    return inode == EXT2_INVALID_INODE ? nullptr : new uint32_t(inode);
}

bool influx::fs::ext2::compare_fs_file_data(void *fs_file_data_1, void *fs_file_data_2) {
    return *(uint32_t *)fs_file_data_1 == *(uint32_t *)fs_file_data_2;
}

influx::structures::dynamic_buffer influx::fs::ext2::read_block(uint32_t block, uint64_t offset,
                                                                int64_t amount) {
    amount = amount == -1 ? _block_size - offset : amount;
    kassert(offset + amount <= _block_size);

    structures::dynamic_buffer buf(amount);

    // Read the block from the drive
    if (!_drive.read((block * _block_size) + offset, amount, buf.data())) {
        return structures::dynamic_buffer();  // Return empty buffer
    }

    return buf;
}

bool influx::fs::ext2::write_block(uint32_t block, influx::structures::dynamic_buffer &buf,
                                   uint64_t offset) {
    kassert(buf.size() <= _block_size - offset);

    // Write the block
    return _drive.write((block * _block_size) + offset, buf.size(), buf.data());
}

bool influx::fs::ext2::clear_block(uint32_t block) {
    structures::dynamic_buffer buf(_block_size);

    // Set the buf as zeros
    memory::utils::memset(buf.data(), 0, _block_size);

    // Write the data to the drive
    return write_block(block, buf);
}

influx::fs::ext2_inode *influx::fs::ext2::get_inode(uint32_t inode) {
    kassert(inode <= _sb.inodes_count && inode > 0);

    uint32_t block_group = (inode - 1) / _sb.inodes_per_group;
    uint32_t inode_index = (inode - 1) % _sb.inodes_per_group;
    uint32_t block_index = inode_index / (_block_size / _sb.inode_size);
    uint32_t inode_block_index = (inode - 1) % (_block_size / _sb.inode_size);
    ext2_inode *inode_obj = new ext2_inode();

    threading::lock_guard lk(_block_groups_mutexes[block_group]);

    // Read the inode
    if (!_drive.read(
            (_block_groups[block_group].inode_table_start_block + block_index) * _block_size +
                inode_block_index * _sb.inode_size,
            sizeof(ext2_inode), inode_obj)) {
        delete inode_obj;
        return nullptr;
    }

    return inode_obj;
}

uint32_t influx::fs::ext2::find_inode(const influx::vfs::path &file_path) {
    uint32_t inode = EXT2_INVALID_INODE, current_inode = EXT2_INVALID_INODE;
    ext2_inode *current_inode_obj = nullptr;

    structures::vector<vfs::dir_entry> dir_entries;

    // For each branch try to find it's matching inode
    for (const auto &branch : file_path.branches()) {
        // Save current inode
        current_inode = inode;

        // Get the inode object of the new inode
        if (inode != EXT2_INVALID_INODE) {
            current_inode_obj = get_inode(inode);
            if (current_inode_obj == nullptr) {
                return EXT2_INVALID_INODE;
            }
        }

        // If the current inode is null, it means the branch must be root
        if (inode == EXT2_INVALID_INODE && branch == "") {
            // Get the root inode
            inode = EXT2_ROOT_INO;
        } else if (current_inode_obj != nullptr &&
                   current_inode_obj->types_permissions &
                       ext2_types_permissions::directory)  // Check that the inode is a directory
        {
            // Read directory
            dir_entries = read_dir(current_inode_obj);

            // Search for the wanted entry
            for (const auto &entry : dir_entries) {
                if (entry.name == branch) {
                    inode = (uint32_t)entry.inode;
                    break;
                }
            }

            // Branch not found
            if (current_inode == inode) {
                return EXT2_INVALID_INODE;
            }
        } else {
            return EXT2_INVALID_INODE;
        }

        // Free previous inode
        if (current_inode_obj != nullptr) {
            delete current_inode_obj;
            current_inode_obj = nullptr;
        }
    }

    // Free inode object
    if (current_inode_obj != nullptr) {
        delete current_inode_obj;
    }

    return inode;
}

uint32_t influx::fs::ext2::get_block_for_offset(influx::fs::ext2_inode *inode, uint64_t offset,
                                                bool allocate) {
    structures::dynamic_buffer buf;

    uint32_t block_index = (uint32_t)(offset / _block_size);
    uint32_t doubly_indirect_block = EXT2_INVALID_BLOCK, doubly_index = 0,
             singly_indirect_block = EXT2_INVALID_BLOCK, singly_index = 0;

    // Check for valid offset
    if ((!allocate && offset >= inode->size) ||
        (allocate && offset >= ((EXT2_TIND_N_BLOCKS + EXT2_DIND_N_BLOCKS + EXT2_IND_N_BLOCKS +
                                 EXT2_NDIR_BLOCKS) *
                                _block_size))) {
        return EXT2_INVALID_BLOCK;
    }

    if (block_index < EXT2_NDIR_BLOCKS) {  // Direct block
        // Allocate new block if needed
        if (allocate && inode->block_pointers[block_index] == EXT2_INVALID_BLOCK) {
            inode->block_pointers[block_index] = alloc_block();
            inode->disk_sectors_count += _block_size / ATA_SECTOR_SIZE;
        }

        return inode->block_pointers[block_index];
    } else if (block_index < (EXT2_IND_N_BLOCKS + EXT2_NDIR_BLOCKS)) {  // Singly indirect block
        // Allocate new block if needed
        if (allocate && inode->block_pointers[EXT2_IND_BLOCK] == EXT2_INVALID_BLOCK) {
            inode->block_pointers[EXT2_IND_BLOCK] = alloc_block();
            inode->disk_sectors_count += _block_size / ATA_SECTOR_SIZE;

            if (inode->block_pointers[EXT2_IND_BLOCK] == EXT2_INVALID_BLOCK ||
                !clear_block(inode->block_pointers[EXT2_IND_BLOCK])) {
                return inode->block_pointers[EXT2_IND_BLOCK];
            }
        }

        singly_indirect_block = inode->block_pointers[EXT2_IND_BLOCK];
        singly_index = block_index - EXT2_NDIR_BLOCKS;
    } else if (block_index < (EXT2_DIND_N_BLOCKS + EXT2_IND_N_BLOCKS +
                              EXT2_NDIR_BLOCKS)) {  // Doubly indirect block
        // Allocate new block if needed
        if (allocate && inode->block_pointers[EXT2_DIND_BLOCK] == EXT2_INVALID_BLOCK) {
            inode->block_pointers[EXT2_DIND_BLOCK] = alloc_block();
            inode->disk_sectors_count += _block_size / ATA_SECTOR_SIZE;

            if (inode->block_pointers[EXT2_DIND_BLOCK] == EXT2_INVALID_BLOCK ||
                !clear_block(inode->block_pointers[EXT2_DIND_BLOCK])) {
                return inode->block_pointers[EXT2_DIND_BLOCK];
            }
        }

        doubly_indirect_block = inode->block_pointers[EXT2_DIND_BLOCK];
        doubly_index = (uint32_t)(block_index - (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS)) /
                       (uint32_t)EXT2_IND_N_BLOCKS;
        singly_index = (uint32_t)(block_index - (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS)) %
                       (uint32_t)EXT2_IND_N_BLOCKS;
    } else {  // Triply indirect block
        // Allocate new block if needed
        if (allocate && inode->block_pointers[EXT2_TIND_BLOCK] == EXT2_INVALID_BLOCK) {
            inode->block_pointers[EXT2_TIND_BLOCK] = alloc_block();
            inode->disk_sectors_count += _block_size / ATA_SECTOR_SIZE;

            if (inode->block_pointers[EXT2_TIND_BLOCK] == EXT2_INVALID_BLOCK ||
                !clear_block(inode->block_pointers[EXT2_TIND_BLOCK])) {
                return inode->block_pointers[EXT2_TIND_BLOCK];
            }
        }

        buf =
            read_block(inode->block_pointers[EXT2_TIND_BLOCK],
                       (block_index - (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS + EXT2_DIND_N_BLOCKS)) /
                           EXT2_DIND_N_BLOCKS,
                       sizeof(uint32_t));
        if (buf.size() == sizeof(uint32_t)) {
            // If there is no block for doubly indirect, allocate one
            if (allocate && *(uint32_t *)buf.data() == EXT2_INVALID_BLOCK) {
                *(uint32_t *)buf.data() = alloc_block();
                inode->disk_sectors_count += _block_size / ATA_SECTOR_SIZE;

                if (*(uint32_t *)buf.data() == EXT2_INVALID_BLOCK ||
                    !clear_block(*(uint32_t *)buf.data()) ||
                    !write_block(inode->block_pointers[EXT2_TIND_BLOCK], buf,
                                 (block_index -
                                  (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS + EXT2_DIND_N_BLOCKS)) /
                                     EXT2_DIND_N_BLOCKS)) {
                    return EXT2_INVALID_BLOCK;
                }
            }

            doubly_indirect_block = *(uint32_t *)buf.data();
            doubly_index = (uint32_t)(block_index -
                                      (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS + EXT2_DIND_N_BLOCKS)) /
                           (uint32_t)EXT2_DIND_N_BLOCKS;
            singly_index = (uint32_t)(block_index -
                                      (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS + EXT2_DIND_N_BLOCKS)) %
                           (uint32_t)EXT2_IND_N_BLOCKS;
        } else {
            return EXT2_INVALID_BLOCK;
        }
    }

    // Handle block in doubly indirect block
    if (doubly_indirect_block != EXT2_INVALID_BLOCK) {
        buf = read_block(doubly_indirect_block, doubly_index * sizeof(uint32_t), sizeof(uint32_t));
        if (buf.size() == sizeof(uint32_t)) {
            // If there is no block for signly indirect, allocate one
            if (allocate && *(uint32_t *)buf.data() == EXT2_INVALID_BLOCK) {
                *(uint32_t *)buf.data() = alloc_block();
                inode->disk_sectors_count += _block_size / ATA_SECTOR_SIZE;

                if (*(uint32_t *)buf.data() == EXT2_INVALID_BLOCK ||
                    !clear_block(*(uint32_t *)buf.data()) ||
                    !write_block(doubly_indirect_block, buf, doubly_index * sizeof(uint32_t))) {
                    return EXT2_INVALID_BLOCK;
                }
            }

            singly_indirect_block = *(uint32_t *)buf.data();
        } else {
            return EXT2_INVALID_BLOCK;
        }
    }

    // Handle block in signly indirect block
    if (singly_indirect_block != EXT2_INVALID_BLOCK) {
        buf = read_block(singly_indirect_block, singly_index * sizeof(uint32_t), sizeof(uint32_t));
        if (buf.size() == sizeof(uint32_t)) {
            // If there is no block, allocate one
            if (allocate && *(uint32_t *)buf.data() == EXT2_INVALID_BLOCK) {
                *(uint32_t *)buf.data() = alloc_block();
                inode->disk_sectors_count += _block_size / ATA_SECTOR_SIZE;

                if (*(uint32_t *)buf.data() == EXT2_INVALID_BLOCK ||
                    !write_block(singly_indirect_block, buf, singly_index * sizeof(uint32_t))) {
                    return EXT2_INVALID_BLOCK;
                }
            }

            return *(uint32_t *)buf.data();
        } else {
            return EXT2_INVALID_BLOCK;
        }
    }

    return EXT2_INVALID_BLOCK;
}

influx::structures::dynamic_buffer influx::fs::ext2::read_file(influx::fs::ext2_inode *inode,
                                                               uint64_t offset, uint64_t amount) {
    kassert(offset + amount <= inode->size);

    structures::dynamic_buffer buf(amount);

    uint64_t current_offset = offset + (_block_size - (offset % _block_size)),
             current_block = get_block_for_offset(inode, current_offset, false);
    uint32_t first_block = get_block_for_offset(inode, offset, false),
             last_block = get_block_for_offset(
                 inode, offset + amount - ((amount + offset) % _block_size), false);
    uint64_t first_block_read_amount = (amount + (offset % _block_size)) > _block_size
                                           ? _block_size - (offset % _block_size)
                                           : amount,
             last_block_read_amount = (amount + offset) % _block_size;

    // Read the first wanted block
    if (first_block != EXT2_INVALID_BLOCK &&
        !_drive.read(first_block * _block_size + (offset % _block_size), first_block_read_amount,
                     buf.data())) {
        return structures::dynamic_buffer();
    }

    // For each complete block, read it
    if (first_block_read_amount != amount) {
        while (current_block != last_block) {
            if (current_block != EXT2_INVALID_BLOCK &&
                !_drive.read(current_block * _block_size, _block_size,
                             buf.data() + (current_offset - offset))) {
                return structures::dynamic_buffer();
            }

            // Get the next block
            current_offset += _block_size;
            current_block = get_block_for_offset(inode, current_offset, false);
        }
    }

    // Read the last block remainder
    if (last_block != first_block) {
        if (last_block != EXT2_INVALID_BLOCK &&
            !_drive.read(last_block * _block_size, last_block_read_amount,
                         buf.data() + (amount - last_block_read_amount))) {
            return structures::dynamic_buffer();
        }
    }

    // Update accessed time of inode
    inode->last_access_time = (uint32_t)kernel::time_manager()->unix_timestamp();

    return buf;
}

uint64_t influx::fs::ext2::write_file(influx::fs::ext2_inode *inode, uint64_t offset,
                                      influx::structures::dynamic_buffer buf) {
    uint64_t amount = buf.size();
    uint64_t current_offset = 0, current_write_amount = 0;

    uint32_t current_block = EXT2_INVALID_BLOCK;
    uint32_t size_change = (uint32_t)(buf.size() - (inode->size - offset));

    // Verify file offset
    if ((offset + buf.size() >=
         ((EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS + EXT2_DIND_N_BLOCKS + EXT2_TIND_N_BLOCKS) *
          _block_size))) {
        return 0;
    }

    // While we didn't write the entire data
    while (current_offset < amount) {
        // Get(allocate if needed) the block for the current offset
        current_block = get_block_for_offset(inode, offset + current_offset, true);
        if (current_block == EXT2_INVALID_BLOCK) {
            break;
        }

        // Calculate current write amount
        current_write_amount = algorithm::min<uint64_t>(
            _block_size - (offset + current_offset) % _block_size, amount - current_offset);

        // Write the data for the block
        if (!_drive.write(current_block * _block_size + (offset + current_offset) % _block_size,
                          current_write_amount, buf.data() + current_offset)) {
            break;
        }

        // Advance to the next block
        current_offset += current_write_amount;
    }

    // Update file's size and disk sectors
    inode->size += (uint32_t)size_change;

    // Update accessed and modified times of inode
    inode->last_access_time = (uint32_t)kernel::time_manager()->unix_timestamp();
    inode->last_modification_time = (uint32_t)kernel::time_manager()->unix_timestamp();

    return current_offset;
}

influx::structures::vector<influx::vfs::dir_entry> influx::fs::ext2::read_dir(
    influx::fs::ext2_inode *dir_inode) {
    kassert(dir_inode->types_permissions & ext2_types_permissions::directory);

    structures::vector<vfs::dir_entry> entries;

    uint64_t buf_offset = 0;
    ext2_dir_entry *dir_entry = nullptr;

    // Read the dir file
    structures::dynamic_buffer buf = read_file(dir_inode, 0, dir_inode->size);
    if (!buf.empty()) {
        // While we didn't reach the end of the dir entry table
        while (buf_offset < dir_inode->size) {
            dir_entry = (ext2_dir_entry *)(buf.data() + buf_offset);

            // If the entry isn't blank
            if (dir_entry->inode != EXT2_INVALID_INODE) {
                // Create the entry for the dir entry
                entries.push_back(vfs::dir_entry{
                    .inode = dir_entry->inode,
                    .type = file_type_for_dir_entry(dir_entry),
                    .name = structures::string((char *)dir_entry->name, dir_entry->name_len)});
            }

            // Increase buf offset
            buf_offset += dir_entry->size;
        }
    }

    return entries;
}

influx::vfs::file_type influx::fs::ext2::file_type_for_dir_entry(
    influx::fs::ext2_dir_entry *dir_entry) {
    // If the type is unknown, read the inode
    if (dir_entry->type == ext2_dir_entry_type::unknown) {
        ext2_inode *inode = get_inode(dir_entry->inode);
        if (inode == nullptr) {
            return vfs::file_type::unknown;
        }

        return file_type_for_inode(inode);
    } else {
        switch (dir_entry->type) {
            case ext2_dir_entry_type::regular:
                return vfs::file_type::regular;
            case ext2_dir_entry_type::directory:
                return vfs::file_type::directory;
            case ext2_dir_entry_type::character_device:
                return vfs::file_type::character_device;
            case ext2_dir_entry_type::block_device:
                return vfs::file_type::block_device;
            case ext2_dir_entry_type::fifo:
                return vfs::file_type::fifo;
            case ext2_dir_entry_type::socket:
                return vfs::file_type::socket;
            case ext2_dir_entry_type::symbolic_link:
                return vfs::file_type::symbolic_link;
            default:
                return vfs::file_type::unknown;
        }
    }

    return vfs::file_type::unknown;
}

influx::vfs::file_type influx::fs::ext2::file_type_for_inode(influx::fs::ext2_inode *inode) {
    // Get type from inode
    switch (inode->types_permissions & EXT2_INODE_TYPES_MASK) {
        case ext2_types_permissions::fifo:
            return vfs::file_type::fifo;
        case ext2_types_permissions::character_device:
            return vfs::file_type::character_device;
        case ext2_types_permissions::directory:
            return vfs::file_type::directory;
        case ext2_types_permissions::block_device:
            return vfs::file_type::block_device;
        case ext2_types_permissions::regular_file:
            return vfs::file_type::regular;
        case ext2_types_permissions::symbolic_link:
            return vfs::file_type::symbolic_link;
        case ext2_types_permissions::unix_socket:
            return vfs::file_type::socket;
        default:
            return vfs::file_type::unknown;
    }
}

influx::vfs::file_permissions influx::fs::ext2::file_permissions_for_inode(
    influx::fs::ext2_inode *inode) {
    vfs::file_permissions permissions;

    // Read permission
    if (inode->types_permissions & ext2_types_permissions::user_read) {
        permissions.read = true;
    }

    // Write permission
    if (inode->types_permissions & ext2_types_permissions::user_write) {
        permissions.write = true;
    }

    // Execute permission
    if (inode->types_permissions & ext2_types_permissions::user_execute) {
        permissions.execute = true;
    }

    return permissions;
}

influx::fs::ext2_types_permissions influx::fs::ext2::ext2_file_types_permissions(
    influx::vfs::file_type type, influx::vfs::file_permissions permissions) {
    ext2_types_permissions types_permissions = (ext2_types_permissions)0;

    // File type
    switch (type) {
        case vfs::file_type::regular:
            types_permissions =
                (ext2_types_permissions)(types_permissions | ext2_types_permissions::regular_file);
            break;

        case vfs::file_type::directory:
            types_permissions =
                (ext2_types_permissions)(types_permissions | ext2_types_permissions::directory);
            break;

        case vfs::file_type::character_device:
            types_permissions = (ext2_types_permissions)(types_permissions |
                                                         ext2_types_permissions::character_device);
            break;

        case vfs::file_type::block_device:
            types_permissions =
                (ext2_types_permissions)(types_permissions | ext2_types_permissions::block_device);
            break;

        case vfs::file_type::fifo:
            types_permissions =
                (ext2_types_permissions)(types_permissions | ext2_types_permissions::fifo);
            break;

        case vfs::file_type::socket:
            types_permissions =
                (ext2_types_permissions)(types_permissions | ext2_types_permissions::unix_socket);
            break;

        case vfs::file_type::symbolic_link:
            types_permissions =
                (ext2_types_permissions)(types_permissions | ext2_types_permissions::symbolic_link);
            break;

        default:
            break;
    }

    // Read permission
    if (permissions.read) {
        types_permissions =
            (ext2_types_permissions)(types_permissions | ext2_types_permissions::user_read);
    }

    // Write permission
    if (permissions.write) {
        types_permissions =
            (ext2_types_permissions)(types_permissions | ext2_types_permissions::user_write);
    }

    // Execute permission
    if (permissions.execute) {
        types_permissions =
            (ext2_types_permissions)(types_permissions | ext2_types_permissions::user_execute);
    }

    return types_permissions;
}

influx::fs::ext2_dir_entry_type influx::fs::ext2::ext2_dir_entry_type_for_file_type(
    influx::vfs::file_type type) {
    switch (type) {
        case vfs::file_type::regular:
            return ext2_dir_entry_type::regular;
        case vfs::file_type::directory:
            return ext2_dir_entry_type::directory;
        case vfs::file_type::character_device:
            return ext2_dir_entry_type::character_device;
        case vfs::file_type::block_device:
            return ext2_dir_entry_type::block_device;
        case vfs::file_type::fifo:
            return ext2_dir_entry_type::fifo;
        case vfs::file_type::socket:
            return ext2_dir_entry_type::socket;
        case vfs::file_type::symbolic_link:
            return ext2_dir_entry_type::symbolic_link;
        default:
            return ext2_dir_entry_type::unknown;
    }
}

uint32_t influx::fs::ext2::create_inode(influx::vfs::file_type type,
                                        influx::vfs::file_permissions permissions) {
    uint32_t inode = alloc_inode();

    ext2_inode *inode_obj = new ext2_inode();

    // If the inode allocation failed
    if (inode == EXT2_INVALID_INODE) {
        delete inode_obj;
        return EXT2_INVALID_INODE;
    }

    // Reset inode object
    memory::utils::memset(inode_obj, 0, sizeof(ext2_inode));

    // Calculate types and permissions for file
    inode_obj->types_permissions = ext2_file_types_permissions(type, permissions);

    // Set creation time, access time and modification time
    inode_obj->creation_time = (uint32_t)kernel::time_manager()->unix_timestamp();
    inode_obj->last_modification_time = inode_obj->creation_time;
    inode_obj->last_access_time = (uint32_t)kernel::time_manager()->unix_timestamp();

    // Set initial hard links count
    inode_obj->hard_links_count = 1;

    // TODO: Set here user and group id

    // Save the new inode
    save_inode(inode, inode_obj);

    // Delete the inode object
    delete inode_obj;

    return inode;
}

influx::vfs::error influx::fs::ext2::create_dir_entry(ext2_inode *dir_inode, uint32_t file_inode,
                                                      influx::fs::ext2_dir_entry_type entry_type,
                                                      influx::structures::string file_name) {
    kassert(file_name.size() <= UINT8_MAX);

    // ** This assumes there is no other file in the directory with the same name **

    uint64_t dir_file_offset = 0, new_dir_entry_offset = dir_inode->size;
    ext2_dir_entry *dir_entry = nullptr;

    uint64_t new_dir_entry_size = sizeof(ext2_dir_entry) + file_name.size();
    structures::dynamic_buffer new_dir_entry_buf = structures::dynamic_buffer(new_dir_entry_size);

    structures::dynamic_buffer buf, prev_dir_entry_buf;

    // Init the ext2 dir entry
    *(ext2_dir_entry *)new_dir_entry_buf.data() = ext2_dir_entry{
        .inode = file_inode, .size = 0, .name_len = (uint8_t)file_name.size(), .type = entry_type};
    memory::utils::memcpy(((ext2_dir_entry *)new_dir_entry_buf.data())->name, file_name.c_str(),
                          file_name.size());

    // Read the dir file
    buf = read_file(dir_inode, 0, dir_inode->size);
    if (!buf.empty()) {
        // While we didn't reach the end of the dir entry table
        while (dir_file_offset < dir_inode->size) {
            dir_entry = (ext2_dir_entry *)(buf.data() + dir_file_offset);

            // If there is enough space for the new dir entry
            if (dir_entry->size - dir_entry->name_len - sizeof(ext2_dir_entry) -
                    (4 - ((dir_file_offset + dir_entry->name_len + sizeof(ext2_dir_entry)) % 4)) >=
                new_dir_entry_size) {
                // Calculate new dir entry offset + align
                new_dir_entry_offset =
                    dir_file_offset + dir_entry->name_len + sizeof(ext2_dir_entry) +
                    (4 - ((dir_file_offset + dir_entry->name_len + sizeof(ext2_dir_entry)) %
                          4));  // Align address (to 4)

                // Check if the new offset spans to 2 blocks
                if (new_dir_entry_offset / _block_size ==
                    (new_dir_entry_offset + new_dir_entry_size) / _block_size) {
                    break;
                }
            }

            // Increase buf offset
            dir_file_offset += dir_entry->size;
        }
    } else {
        return vfs::error::unknown_error;
    }

    // If no empty space was found
    if (dir_file_offset == dir_inode->size) {
        new_dir_entry_offset = dir_inode->size + (_block_size - (dir_inode->size % _block_size));
        ((ext2_dir_entry *)new_dir_entry_buf.data())->size =
            (uint16_t)(_block_size - new_dir_entry_size);
    } else {
        // Set the size of the new dir entry
        ((ext2_dir_entry *)new_dir_entry_buf.data())->size =
            (uint16_t)(dir_file_offset + dir_entry->size - new_dir_entry_offset);

        // Set the new size of the prev dir entry
        dir_entry->size = (uint16_t)(new_dir_entry_offset - dir_file_offset);

        // Create a copy of the prev dir entry
        prev_dir_entry_buf =
            structures::dynamic_buffer(sizeof(ext2_dir_entry) + dir_entry->name_len);
        memory::utils::memcpy(prev_dir_entry_buf.data(), dir_entry,
                              sizeof(ext2_dir_entry) + dir_entry->name_len);

        // Try to write the prev dir entry
        if (!write_file(dir_inode, dir_file_offset, prev_dir_entry_buf)) {
            return vfs::error::io_error;
        }
    }

    // Try to write the new dir entry
    if (!write_file(dir_inode, new_dir_entry_offset, new_dir_entry_buf)) {
        return vfs::error::io_error;
    }

    return vfs::error::success;
}

uint32_t influx::fs::ext2::alloc_block() {
    uint32_t block_group = (uint32_t)_block_groups.size();
    uint64_t block_index = EXT2_INVALID_BLOCK;

    structures::dynamic_buffer bitmap_buf;
    structures::bitmap<uint8_t> block_bitmap;

    {
        threading::lock_guard sb_lk(_sb_mutex);

        // Check if there are any free blocks
        if (_sb.free_blocks_count > 0) {
            _sb.free_blocks_count--;
            if (!_drive.write(EXT2_SUPERBLOCK_OFFSET, sizeof(_sb), &_sb)) {
                return EXT2_INVALID_BLOCK;
            }
        } else {
            return EXT2_INVALID_BLOCK;
        }
    }

    // Search for a block group with free blocks
    for (uint32_t i = 0; i < _block_groups.size(); i++) {
        threading::lock_guard lk(_block_groups_mutexes[i]);

        // If the block group has free blocks, set it as the block group
        if (_block_groups[i].free_blocks_count > 0) {
            block_group = i;
            break;
        }
    }

    // If no block group found
    if (block_group == _block_groups.size()) {
        return EXT2_INVALID_BLOCK;
    }

    threading::lock_guard lk(_block_groups_mutexes[block_group]);

    // Get block bitmap
    bitmap_buf = read_block(_block_groups[block_group].block_bitmap_block);
    block_bitmap = structures::bitmap<uint8_t>(bitmap_buf.data(), _sb.blocks_per_group, false);
    if (bitmap_buf.empty()) {
        return EXT2_INVALID_BLOCK;
    }

    // Get a free block
    if (!block_bitmap.search_bit(0, block_index)) {
        // We shouldn't get here
        _log("No free blocks found in block group %d where it should have %d free blocks!\n",
             block_group, _block_groups[block_group].free_blocks_count);

        // Update the block group free blocks count to 0
        _block_groups[block_group].free_blocks_count = 0;
        save_block_group(block_group);

        return EXT2_INVALID_BLOCK;
    }

    // Set the block as allocated
    block_bitmap[block_index] = true;

    // Write the bitmap to the block bitmap block
    if (!write_block(_block_groups[block_group].block_bitmap_block, bitmap_buf)) {
        return EXT2_INVALID_BLOCK;
    }

    // Decrease the amount of free blocks in the block group and update it
    _block_groups[block_group].free_blocks_count--;
    if (!save_block_group(block_group)) {
        return EXT2_INVALID_BLOCK;
    }

    return block_group * _sb.blocks_per_group + (uint32_t)block_index;
}

uint32_t influx::fs::ext2::alloc_inode() {
    uint32_t block_group = (uint32_t)_block_groups.size();
    uint64_t inode_index = EXT2_INVALID_INODE;

    structures::dynamic_buffer bitmap_buf;
    structures::bitmap<uint8_t> inode_bitmap;

    {
        threading::lock_guard sb_lk(_sb_mutex);

        // Check if there are any free inodes
        if (_sb.free_inodes_count > 0) {
            _sb.free_inodes_count--;
            if (!_drive.write(EXT2_SUPERBLOCK_OFFSET, sizeof(_sb), &_sb)) {
                return EXT2_INVALID_INODE;
            }
        } else {
            return EXT2_INVALID_INODE;
        }
    }

    // Search for a block group with free inodes
    for (uint32_t i = 0; i < _block_groups.size(); i++) {
        threading::lock_guard lk(_block_groups_mutexes[i]);

        // If the block group has free inodes, set it as the block group
        if (_block_groups[i].free_inodes_count > 0) {
            block_group = i;
            break;
        }
    }

    // If no block group found
    if (block_group == _block_groups.size()) {
        return EXT2_INVALID_INODE;
    }

    threading::lock_guard lk(_block_groups_mutexes[block_group]);

    // Get block bitmap
    bitmap_buf = read_block(_block_groups[block_group].inode_bitmap_block);
    inode_bitmap = structures::bitmap<uint8_t>(bitmap_buf.data(), _sb.inodes_per_group, false);
    if (bitmap_buf.empty()) {
        return EXT2_INVALID_INODE;
    }

    // Get a free inode
    if (!inode_bitmap.search_bit(0, inode_index)) {
        // We shouldn't get here
        _log("No free inodes found in block group %d where it should have %d free inodes!\n",
             block_group, _block_groups[block_group].free_inodes_count);

        // Update the block group free inodes count to 0
        _block_groups[block_group].free_inodes_count = 0;
        save_block_group(block_group);

        return EXT2_INVALID_INODE;
    }

    // Set the inode as allocated
    inode_bitmap[inode_index] = true;

    // Write the bitmap to the inode bitmap block
    if (!write_block(_block_groups[block_group].inode_bitmap_block, bitmap_buf)) {
        return EXT2_INVALID_INODE;
    }

    // Decrease the amount of free inodes in the block group and update it
    _block_groups[block_group].free_inodes_count--;
    if (!save_block_group(block_group)) {
        return EXT2_INVALID_INODE;
    }

    return block_group * _sb.inodes_per_group + (uint32_t)inode_index + 1;
}

bool influx::fs::ext2::free_block(uint32_t block) {
    uint32_t block_group = block / _sb.blocks_per_group;
    uint32_t block_index = block % _sb.blocks_per_group;

    structures::dynamic_buffer bitmap_buf;
    structures::bitmap<uint8_t> block_bitmap;

    threading::lock_guard lk(_block_groups_mutexes[block_group]);

    // Read block bitmap
    bitmap_buf = read_block(_block_groups[block_group].block_bitmap_block);
    block_bitmap = structures::bitmap<uint8_t>(bitmap_buf.data(), _sb.blocks_per_group, false);
    if (bitmap_buf.empty()) {
        return false;
    }

    // If the block is already free
    if (block_bitmap[block_index] == false) {
        return true;
    }

    // Free the block
    block_bitmap[block_index] = false;

    // Write the bitmap to the block bitmap block
    if (!write_block(_block_groups[block_group].block_bitmap_block, bitmap_buf)) {
        return false;
    }

    // Increase the amount of free blocks in the block group and update it
    _block_groups[block_group].free_blocks_count++;
    if (!save_block_group(block_group)) {
        return false;
    }

    {
        threading::lock_guard sb_lk(_sb_mutex);

        // Increase the amount of free blocks
        _sb.free_blocks_count++;
        if (!_drive.write(EXT2_SUPERBLOCK_OFFSET, sizeof(_sb), &_sb)) {
            return false;
        }
    }

    return true;
}

bool influx::fs::ext2::free_inode(uint32_t inode) {
    uint32_t block_group = (inode - 1) / _sb.inodes_per_group;
    uint32_t inode_index = (inode - 1) % _sb.inodes_per_group;
    uint32_t block_index = inode_index / (_block_size / _sb.inode_size);
    uint32_t inode_block_index = (inode - 1) % (_block_size / _sb.inode_size);

    structures::dynamic_buffer bitmap_buf, inode_table_buf;
    structures::bitmap<uint8_t> inode_bitmap;

    threading::lock_guard lk(_block_groups_mutexes[block_group]);

    // Read inode bitmap
    bitmap_buf = read_block(_block_groups[block_group].inode_bitmap_block);
    inode_bitmap = structures::bitmap<uint8_t>(bitmap_buf.data(), _sb.inodes_per_group, false);
    if (bitmap_buf.empty()) {
        return false;
    }

    // If the inode is already free
    if (inode_bitmap[inode_index] == false) {
        return true;
    }

    // Free the inode
    inode_bitmap[inode_index] = false;

    // Write the bitmap to the inode bitmap block
    if (!write_block(_block_groups[block_group].inode_bitmap_block, bitmap_buf)) {
        return false;
    }

    // Increase the amount of free inodes in the block group and update it
    _block_groups[block_group].free_inodes_count++;
    if (!save_block_group(block_group)) {
        return false;
    }

    // Read the inode table block
    inode_table_buf = read_block(_block_groups[block_group].inode_table_start_block + block_index);
    if (inode_table_buf.empty()) {
        return false;
    }

    // Clear the inode
    memory::utils::memset(inode_table_buf.data() + _sb.inode_size * inode_block_index, 0,
                          sizeof(ext2_inode));

    // Write the inode table block back
    if (!write_block(_block_groups[block_group].inode_table_start_block + block_index,
                     inode_table_buf)) {
        return false;
    }

    {
        threading::lock_guard sb_lk(_sb_mutex);

        // Increase the amount of free inode
        _sb.free_inodes_count++;
        if (!_drive.write(EXT2_SUPERBLOCK_OFFSET, sizeof(_sb), &_sb)) {
            return false;
        }
    }

    return true;
}

bool influx::fs::ext2::save_block_group(uint32_t block_group) {
    // Mutex for block group should be taken already

    return _drive.write(
        _block_size * EXT2_BGDT_BLOCK + (block_group * sizeof(ext2_block_group_desc)),
        sizeof(ext2_block_group_desc), &_block_groups[block_group]);
}

bool influx::fs::ext2::save_inode(uint32_t inode, influx::fs::ext2_inode *inode_obj) {
    kassert(inode <= _sb.inodes_count);

    uint32_t block_group = (inode - 1) / _sb.inodes_per_group;
    uint32_t inode_index = (inode - 1) % _sb.inodes_per_group;
    uint32_t block_index = inode_index / (_block_size / _sb.inode_size);
    uint32_t inode_block_index = (inode - 1) % (_block_size / _sb.inode_size);

    threading::lock_guard lk(_block_groups_mutexes[block_group]);

    // Write the inode
    if (!_drive.write(
            (_block_groups[block_group].inode_table_start_block + block_index) * _block_size +
                inode_block_index * _sb.inode_size,
            sizeof(ext2_inode), inode_obj)) {
        return false;
    }

    return true;
}