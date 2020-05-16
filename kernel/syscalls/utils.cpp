#include <kernel/memory/paging_manager.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::utils::convert_vfs_error(influx::vfs::error err) {
    switch (err) {
        case vfs::error::success:
            return 0;

        case vfs::error::file_is_directory:
            return -EISDIR;

        case vfs::error::file_is_not_directory:
            return -ENOTDIR;

        case vfs::error::invalid_file:
            return -EBADF;

        case vfs::error::file_not_found:
            return -ENOENT;

        case vfs::error::insufficient_permissions:
            return -EACCES;

        case vfs::error::invalid_file_access:
            return -EBADF;

        case vfs::error::invalid_flags:
            return -EINVAL;

        case vfs::error::quota_exhausted:
            return -EDQUOT;

        case vfs::error::file_already_exists:
            return -EEXIST;

        case vfs::error::invalid_position:
            return -EINVAL;

        case vfs::error::interrupted:
            return -EINTR;

        case vfs::error::is_pipe:
            return -ESPIPE;

        case vfs::error::pipe_closed:
            return -EPIPE;

        case vfs::error::unknown_error:
        case vfs::error::io_error:
        default:
            return -EIO;
    }
}

bool influx::syscalls::utils::is_string_in_user_memory(const char *str) {
    // Check if surpassing the userland memory barrier
    if ((uint64_t)str > USERLAND_MEMORY_BARRIER) {
        return false;
    }

    // Check initial page of the string
    if (!(memory::paging_manager::get_pte_permissions((uint64_t)str) & PROT_READ)) {
        return false;
    }

    // While we didn't reach the end of the string
    while (*str) {
        // Move to the next character
        str++;

        // Check every new page
        if ((uint64_t)str % PAGE_SIZE == 0 &&
            ((uint64_t)str > USERLAND_MEMORY_BARRIER ||
             !(memory::paging_manager::get_pte_permissions((uint64_t)str) & PROT_READ))) {
            return false;
        }
    }

    return true;
}

bool influx::syscalls::utils::is_buffer_in_user_memory(const void *buf, uint64_t size,
                                                       protection_flags_t permissions) {
    // Check if surpassing the userland memory barrier
    if ((uint64_t)buf > USERLAND_MEMORY_BARRIER ||
        ((uint64_t)buf + size) > USERLAND_MEMORY_BARRIER) {
        return false;
    }

    // Check each page of the buffer
    for (uint64_t addr = (uint64_t)buf; addr < ((uint64_t)buf + size);
         addr += PAGE_SIZE - (addr % PAGE_SIZE)) {
        // Check if the address is mapped and has the correct permissions
        if (!(memory::paging_manager::get_pte_permissions(addr) & permissions)) {
            return false;
        }
    }

    return true;
}