#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::chdir(const char *dir_path) {
    vfs::error err;

    // Verify dir name buffer
    if (!utils::is_string_in_user_memory(dir_path)) {
        return -EFAULT;
    }

    // Try to set working directory
    err = (vfs::error)kernel::scheduler()->set_working_dir(dir_path);

    return utils::convert_vfs_error(err);
}