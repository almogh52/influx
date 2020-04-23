#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::stat(const char *file_path, influx::vfs::file_info *file_info) {
    vfs::error err;

    // Check valid file path and file info
    if (!utils::is_string_in_user_memory(file_path) ||
        !utils::is_buffer_in_user_memory(file_info, sizeof(vfs::file_info),
                                         PROT_READ | PROT_WRITE)) {
        return -EFAULT;
    }

    // Try to get the stat of the file
    err = (vfs::error)kernel::vfs()->stat(file_path, *file_info);

    return utils::convert_vfs_error(err);
}