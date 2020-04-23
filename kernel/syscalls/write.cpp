#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::write(size_t fd, const void *buf, size_t count) {
    int64_t write;

    // Verify valid file descriptor
    if (fd < 3) {
        return -EBADF;
    }

    // Check valid buffer
    if (!utils::is_buffer_in_user_memory(buf, count, PROT_READ)) {
        return -EFAULT;
    }

    // Try to write to the file
    write = kernel::vfs()->write(fd, buf, count);

    // Handle errors
    if (write < 0) {
        return utils::convert_vfs_error((vfs::error)write);
    }

    return write;
}