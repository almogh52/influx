#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::read(size_t fd, void *buf, size_t count) {
    int64_t read;

    // Verify valid file descriptor
    if (fd < 3) {
        return -EBADF;
    }

    // Check valid buffer
    if (!utils::is_buffer_in_user_memory(buf, count, PROT_WRITE)) {
        return -EFAULT;
    }

    // Try to read from the file
    read = kernel::vfs()->read(fd, buf, count);

    // Handle errors
    if (read < 0) {
        return utils::convert_vfs_error((vfs::error)read);
    }

    return read;
}