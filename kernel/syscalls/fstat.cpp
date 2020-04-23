#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::fstat(size_t fd, influx::vfs::file_info *file_info) {
    vfs::error err;

    // Verify valid file descriptor
    if (fd < 3) {
        return -EBADF;
    }

    // Check valid file info
    if (!utils::is_buffer_in_user_memory(file_info, sizeof(vfs::file_info),
                                         PROT_READ | PROT_WRITE)) {
        return -EFAULT;
    }

    // Try to get the stat of the file
    err = (vfs::error)kernel::vfs()->stat(fd, *file_info);

    return utils::convert_vfs_error(err);
}