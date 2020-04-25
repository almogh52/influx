#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::close(size_t fd) {
    vfs::error err;

    // Try to close the file
    err = (vfs::error)kernel::vfs()->close(fd);

    return utils::convert_vfs_error(err);
}