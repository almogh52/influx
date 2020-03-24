#include <kernel/fs/ext2/ext2.h>

#include <kernel/memory/utils.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/unique_lock.h>

#define EXT2_IND_N_BLOCKS (_block_size / sizeof(uint32_t))
#define EXT2_DIND_N_BLOCKS (EXT2_IND_N_BLOCKS * EXT2_IND_N_BLOCKS)

influx::fs::ext2::ext2(const influx::drivers::ata::drive_slice &drive)
    : vfs::filesystem("EXT2", drive), _block_size(0) {}

bool influx::fs::ext2::mount() {
    uint64_t number_of_groups = 0;

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
    _log("Total of %d groups.\n", number_of_groups);

    // Read the block group descriptor table
    _log("Reading block group descriptor table..\n");
    _block_groups.resize(number_of_groups);
    _block_groups_mutexes.resize(number_of_groups);
    if (!_drive.read(_block_size * EXT2_BGDT_BLOCK,
                     sizeof(ext2_block_group_desc) * number_of_groups, _block_groups.data())) {
        _log("Unable to read block group descriptor table from drive!\n");
        return false;
    }

    return true;
}

influx::structures::dynamic_buffer influx::fs::ext2::read_block(uint64_t block, uint64_t offset,
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

bool influx::fs::ext2::write_block(uint64_t block, influx::structures::dynamic_buffer &buf,
                                   uint64_t offset) {
    kassert(buf.size() <= _block_size - offset);

    // Write the block
    return _drive.write((block * _block_size) + offset, buf.size(), buf.data());
}

influx::fs::ext2_inode *influx::fs::ext2::get_inode(uint64_t inode) {
    kassert(inode <= _sb.inodes_count);

    uint64_t block_group = (inode - 1) / _sb.inodes_per_group;
    uint64_t inode_index = (inode - 1) % _sb.inodes_per_group;
    uint64_t block_index = inode_index / (_block_size / _sb.inode_size);
    uint64_t inode_block_index = (inode - 1) % (_block_size / _sb.inode_size);
    ext2_inode *inode_obj = new ext2_inode();

    structures::dynamic_buffer buf;

    threading::unique_lock lk(_block_groups_mutexes[block_group]);

    // Read the inode table block
    buf = read_block(_block_groups[block_group].inode_table_start_block + block_index);
    lk.unlock();

    // Copy the inode data from the inode table
    memory::utils::memcpy(inode_obj, buf.data() + _sb.inode_size * inode_block_index,
                          sizeof(ext2_inode));

    return inode_obj;
}

uint64_t influx::fs::ext2::find_inode(const influx::vfs::path &file_path) {
    uint64_t inode = EXT2_INVALID_INODE, current_inode = EXT2_INVALID_INODE;
    ext2_inode *current_inode_obj = nullptr;

    structures::vector<vfs::dir_entry> dir_entries;

    // For each branch try to find it's matching inode
    for (const auto &branch : file_path.branches()) {
        // Save current inode
        current_inode = inode;

        // Get the inode object of the new inode
        if (inode != EXT2_INVALID_INODE) {
            current_inode_obj = get_inode(inode);
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
                    inode = entry.inode;
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

uint64_t influx::fs::ext2::get_block_for_offset(influx::fs::ext2_inode *inode, uint64_t offset) {
    structures::dynamic_buffer buf;

    // Check for valid offset
    if (offset >= inode->size) {
        return EXT2_INVALID_BLOCK;
    }

    uint64_t block_index = offset / _block_size;
    uint64_t doubly_indirect_block = EXT2_INVALID_BLOCK, doubly_index = 0,
             singly_indirect_block = EXT2_INVALID_BLOCK, singly_index = 0;

    if (block_index < EXT2_NDIR_BLOCKS) {  // Direct block
        return inode->block_pointers[block_index];
    } else if (block_index < (EXT2_IND_N_BLOCKS + EXT2_NDIR_BLOCKS)) {  // Singly indirect block
        singly_indirect_block = inode->block_pointers[EXT2_IND_BLOCK];
        singly_index = block_index - EXT2_NDIR_BLOCKS;
    } else if (block_index < (EXT2_DIND_N_BLOCKS + EXT2_IND_N_BLOCKS +
                              EXT2_NDIR_BLOCKS)) {  // Doubly indirect block
        doubly_indirect_block = inode->block_pointers[EXT2_DIND_BLOCK];
        doubly_index = (block_index - (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS)) / EXT2_IND_N_BLOCKS;
        singly_index = (block_index - (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS)) % EXT2_IND_N_BLOCKS;
    } else {  // Triply indirect block
        buf =
            read_block(inode->block_pointers[EXT2_TIND_BLOCK],
                       (block_index - (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS + EXT2_DIND_N_BLOCKS)) /
                           EXT2_DIND_N_BLOCKS,
                       sizeof(uint32_t));
        if (buf.size() == sizeof(uint32_t)) {
            doubly_indirect_block = *(uint32_t *)buf.data();
            doubly_index =
                (block_index - (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS + EXT2_DIND_N_BLOCKS)) /
                EXT2_DIND_N_BLOCKS;
            singly_index =
                (block_index - (EXT2_NDIR_BLOCKS + EXT2_IND_N_BLOCKS + EXT2_DIND_N_BLOCKS)) %
                EXT2_IND_N_BLOCKS;
        } else {
            return EXT2_INVALID_BLOCK;
        }
    }

    // Handle block in doubly indirect block
    if (doubly_indirect_block != EXT2_INVALID_BLOCK) {
        buf = read_block(doubly_indirect_block, doubly_index * sizeof(uint32_t), sizeof(uint32_t));
        if (buf.size() == sizeof(uint32_t)) {
            singly_indirect_block = *(uint32_t *)buf.data();
        } else {
            return EXT2_INVALID_BLOCK;
        }
    }

    // Handle block in signly indirect block
    if (singly_indirect_block != EXT2_INVALID_BLOCK) {
        buf = read_block(singly_indirect_block, singly_index * sizeof(uint32_t), sizeof(uint32_t));
        if (buf.size() == sizeof(uint32_t)) {
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
             current_block = get_block_for_offset(inode, current_offset);
    uint64_t first_block = get_block_for_offset(inode, offset),
             last_block =
                 get_block_for_offset(inode, offset + amount - ((amount + offset) % _block_size));
    uint64_t first_block_read_amount = (amount + (offset % _block_size)) > _block_size
                                           ? _block_size - (offset % _block_size)
                                           : amount,
             last_block_read_amount = (amount + offset) % _block_size;

    // Verify valid file blocks
    if (first_block == EXT2_INVALID_BLOCK ||
        (last_block_read_amount > 0 && last_block == EXT2_INVALID_BLOCK)) {
        return structures::dynamic_buffer();
    }

    // Read the first wanted block
    if (!_drive.read(first_block * _block_size + (offset % _block_size), first_block_read_amount,
                     buf.data())) {
        return structures::dynamic_buffer();
    }

    // For each complete block, read it
    while (current_block != last_block && first_block_read_amount != amount) {
        if (!_drive.read(current_block * _block_size, _block_size,
                         buf.data() + (current_offset - offset))) {
            return structures::dynamic_buffer();
        }

        // Get the next block
        current_offset += _block_size;
        current_block = get_block_for_offset(inode, current_offset);
        if (current_offset == EXT2_INVALID_BLOCK) {
            return structures::dynamic_buffer();
        }
    }

    // Read the last block remainder
    if (last_block != first_block) {
        if (!_drive.read(last_block * _block_size, last_block_read_amount,
                         buf.data() + (amount - last_block_read_amount))) {
            return structures::dynamic_buffer();
        }
    }

    return buf;
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

            // Create the entry for the dir entry
            entries.push_back(vfs::dir_entry{
                .inode = dir_entry->inode,
                .type = file_type_for_dir_entry(dir_entry),
                .name = structures::string((char *)dir_entry->name, dir_entry->name_len)});

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
        return file_type_for_inode(get_inode(dir_entry->inode));
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
    switch ((ext2_types_permissions)((uint8_t)inode->types_permissions & EXT2_INODE_TYPES_MASK)) {
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

uint64_t influx::fs::ext2::alloc_block() {
    uint64_t block_group = _block_groups.size();
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
    for (uint64_t i = 0; i < _block_groups.size(); i++) {
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

    return block_group * _sb.blocks_per_group + block_index;
}

uint64_t influx::fs::ext2::alloc_inode() {
    uint64_t block_group = _block_groups.size();
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
    for (uint64_t i = 0; i < _block_groups.size(); i++) {
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

    return block_group * _sb.inodes_per_group + inode_index + 1;
}

bool influx::fs::ext2::free_block(uint64_t block) {
    uint64_t block_group = block / _sb.blocks_per_group;
    uint64_t block_index = block % _sb.blocks_per_group;

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

bool influx::fs::ext2::free_inode(uint64_t inode) {
    uint64_t block_group = (inode - 1) / _sb.inodes_per_group;
    uint64_t inode_index = (inode - 1) % _sb.inodes_per_group;
    uint64_t block_index = inode_index / (_block_size / _sb.inode_size);
    uint64_t inode_block_index = (inode - 1) % (_block_size / _sb.inode_size);

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

bool influx::fs::ext2::save_block_group(uint64_t block_group) {
    // Mutex for block group should be taken already

    return _drive.write(
        _block_size * EXT2_BGDT_BLOCK + (block_group * sizeof(ext2_block_group_desc)),
        sizeof(ext2_block_group_desc), &_block_groups[block_group]);
}