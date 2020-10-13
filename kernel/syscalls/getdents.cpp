#include <dirent.h>
#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

unsigned char convert_vfs_file_type(influx::vfs::file_type type) {
    switch (type) {
        case influx::vfs::file_type::regular:
            return DT_REG;

        case influx::vfs::file_type::directory:
            return DT_DIR;

        case influx::vfs::file_type::character_device:
            return DT_CHR;

        case influx::vfs::file_type::block_device:
            return DT_BLK;

        case influx::vfs::file_type::fifo:
            return DT_FIFO;

        case influx::vfs::file_type::socket:
            return DT_SOCK;

        case influx::vfs::file_type::symbolic_link:
            return DT_LNK;

        default:
        case influx::vfs::file_type::unknown:
            return DT_UNKNOWN;
    }
}

int64_t influx::syscalls::handlers::getdents(size_t fd, dirent* dirp, uint64_t count) {
    int64_t read;
    structures::vector<vfs::dir_entry> entries;

    uint64_t buf_use_amount = 0;

    // Check valid buffer
    if (!utils::is_buffer_in_user_memory(dirp, count, PROT_WRITE)) {
        return -EFAULT;
    }

    // Try to read the dir entries
    read = kernel::vfs()->get_dir_entries(fd, entries, count);

    // Handle VFS errors
    if (read < 0) {
        return utils::convert_vfs_error((vfs::error)read);
    }

    // For each VFS dir entry, create dirent struct
    for (const auto& entry : entries) {
        dirp->d_ino = entry.inode;
        dirp->d_type = convert_vfs_file_type(entry.type);
        dirp->d_reclen = (unsigned short)(sizeof(dirent) + entry.name.size() + 1);
        dirp->d_off = dirp->d_reclen;
        buf_use_amount += dirp->d_reclen;

        // Copy entry name
        memory::utils::memcpy(dirp->d_name, entry.name.c_str(), entry.name.size() + 1);

        // Increase directory ptr
        dirp = (dirent*)((uint8_t*)dirp + dirp->d_off);
    }

    return buf_use_amount;
}