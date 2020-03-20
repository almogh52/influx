#include <kernel/fs/ext2/ext2.h>

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
    _drive.read(_block_size * 1,
                sizeof(ext2_block_group_desc) * number_of_groups, _block_groups.data());

    return true;
}