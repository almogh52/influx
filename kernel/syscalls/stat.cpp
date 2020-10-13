#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

influx::syscalls::stat convert_file_info_to_stat(influx::vfs::file_info &info);

int64_t influx::syscalls::handlers::stat(const char *file_name, influx::syscalls::stat *stat) {
    vfs::path file_path;
    vfs::file_info info;
    vfs::error err;

    // Check valid file path and file info
    if (!utils::is_string_in_user_memory(file_name) ||
        !utils::is_buffer_in_user_memory(stat, sizeof(stat), PROT_READ | PROT_WRITE)) {
        return -EFAULT;
    }

    // If the file path is relative, try to make it absolute
    file_path = vfs::path(file_name);
    if (file_path.is_relative()) {
        file_path = kernel::scheduler()->get_working_dir() + file_name;
    }

    // Try to get the stat of the file
    err = (vfs::error)kernel::vfs()->stat(file_path, info);

    // Convert file info to stat struct
    if (err == vfs::error::success) {
        *stat = convert_file_info_to_stat(info);
    }

    return utils::convert_vfs_error(err);
}