#include <kernel/kernel.h>
#include <kernel/memory/paging_manager.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

bool convert_string_array_to_vector(const char **arr,
                                    influx::structures::vector<influx::structures::string> &vec) {
    uint64_t current_page = (uint64_t)arr / PAGE_SIZE;

    // Check if surpassing the userland memory barrier
    if ((uint64_t)arr > USERLAND_MEMORY_BARRIER) {
        return false;
    }

    // Check initial page of the array
    if (!(influx::memory::paging_manager::get_pte_permissions((uint64_t)arr) & PROT_READ)) {
        return false;
    }

    // While we didn't reach the end of the array
    while (*arr) {
        // Check the string
        if (!influx::syscalls::utils::is_string_in_user_memory(*arr)) {
            return false;
        }

        // Insert the string to the array
        vec.push_back(influx::structures::string(*arr));

        // Move to the next string
        arr++;

        // Check every new page
        if ((uint64_t)arr / PAGE_SIZE != current_page) {
            current_page = (uint64_t)arr / PAGE_SIZE;

            if ((uint64_t)arr > USERLAND_MEMORY_BARRIER ||
                !(influx::memory::paging_manager::get_pte_permissions((uint64_t)arr) & PROT_READ)) {
                return false;
            }
        }
    }

    return true;
}

int64_t influx::syscalls::handlers::execve(const char *exec_path, const char **argv,
                                           const char **envp) {
    int64_t err = 0, fd = 0;
    vfs::file_info file;

    structures::vector<structures::string> args, env;

    // Verify exec path string, args and env
    if (!utils::is_string_in_user_memory(exec_path) ||
        !convert_string_array_to_vector(argv, args) || !convert_string_array_to_vector(envp, env)) {
        return -EFAULT;
    }

    // Try to open the file
    fd = kernel::vfs()->open(exec_path, vfs::open_flags::read);

    // Handle errors
    if (fd < 0) {
        return utils::convert_vfs_error((vfs::error)fd);
    }

    // Check that the file is executable
    if ((err = kernel::vfs()->stat(fd, file)) != vfs::error::success || !file.permissions.execute) {
        return -EACCES;
    }

    // Execute the file
    kernel::scheduler()->exec(fd, vfs::path(exec_path).base_name(), args, env);

    return -EFAULT;
}