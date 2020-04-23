#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::unlink(const char *file_path) {
    vfs::error err;

    // Verify file name buffer
    if (!utils::is_string_in_user_memory(file_path)) {
        return -EFAULT;
    }

    // Try to close the file
    err = (vfs::error)kernel::vfs()->unlink(file_path);

    // Handle errors
    if (err < 0) {
        return utils::convert_vfs_error(err);
    }

    return 0;
}