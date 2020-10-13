#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

extern influx::vfs::file_permissions convert_permissions(int mode);

int64_t influx::syscalls::handlers::mkdir(const char *dir_name, int mode) {
    vfs::path dir_path;
    vfs::error err;

    // Verify mode
    if (mode & ~(S_IRWXU | S_IRWXG | S_IRWXO)) {
        return -EINVAL;
    }

    // Verify dir name buffer
    if (!utils::is_string_in_user_memory(dir_name)) {
        return -EFAULT;
    }

    // If the file path is relative, try to make it absolute
    dir_path = vfs::path(dir_name);
    if (dir_path.is_relative()) {
        dir_path = kernel::scheduler()->get_working_dir() + dir_name;
    }

    // Try to create the dir
    err = (vfs::error)kernel::vfs()->mkdir(dir_path, convert_permissions(mode));

    return utils::convert_vfs_error(err);
}