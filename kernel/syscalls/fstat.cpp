#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

uint32_t convert_type_permissions_to_mode(influx::vfs::file_type type,
                                          influx::vfs::file_permissions permissions) {
    uint32_t mode = 0;

    // File type
    switch (type) {
        case influx::vfs::file_type::regular:
            mode |= S_IFREG;
            break;

        case influx::vfs::file_type::directory:
            mode |= S_IFDIR;
            break;

        case influx::vfs::file_type::character_device:
            mode |= S_IFCHR;
            break;

        case influx::vfs::file_type::block_device:
            mode |= S_IFBLK;
            break;

        case influx::vfs::file_type::fifo:
            mode |= S_IFIFO;
            break;

        case influx::vfs::file_type::socket:
            mode |= S_IFSOCK;
            break;

        case influx::vfs::file_type::symbolic_link:
            mode |= S_IFLNK;
            break;

        default:
            break;
    }

    // Read permission
    if (permissions.read) {
        mode |= S_IRUSR;
    }

    // Write permission
    if (permissions.write) {
        mode |= S_IWUSR;
    }

    // Execute permission
    if (permissions.execute) {
        mode |= S_IXUSR;
    }

    return mode;
}

influx::syscalls::stat convert_file_info_to_stat(influx::vfs::file_info &info) {
    return influx::syscalls::stat{
        .device_id = 0,
        .inode = info.inode,
        .mode = convert_type_permissions_to_mode(info.type, info.permissions),
        .hard_links_count = info.hard_links_count,
        .owner_user_id = info.owner_user_id,
        .owner_group_id = info.owner_group_id,
        .rdevice_id = 0,
        .size = info.size,
        .last_access =
            influx::time::timeval{.seconds = info.accessed, .useconds = info.accessed * 1000000},
        .last_modify =
            influx::time::timeval{.seconds = info.modified, .useconds = info.modified * 1000000},
        .creation_time =
            influx::time::timeval{.seconds = info.created, .useconds = info.created * 1000000},
        .fs_block_size = info.fs_block_size,
        .blocks = info.blocks};
}

int64_t influx::syscalls::handlers::fstat(size_t fd, influx::syscalls::stat *stat) {
    vfs::file_info info;
    vfs::error err;

    // Check valid file info
    if (!utils::is_buffer_in_user_memory(stat, sizeof(stat), PROT_READ | PROT_WRITE)) {
        return -EFAULT;
    }

    // Try to get the stat of the file
    err = (vfs::error)kernel::vfs()->stat(fd, info);

    // Convert file info to stat struct
    if (err == vfs::error::success) {
        *stat = convert_file_info_to_stat(info);
    }

    return utils::convert_vfs_error(err);
}