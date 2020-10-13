#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

char *influx::syscalls::handlers::getcwd(char *buf, size_t size) {
    structures::string cwd = kernel::scheduler()->get_working_dir();

    // Check valid buffer
    if (!utils::is_buffer_in_user_memory(buf, size, PROT_WRITE)) {
        return (char *)-EFAULT;
    }

    // If buffer is too small
    if (cwd.size() + 1 > size) {
        return (char *)-ERANGE;
    }

    // Copy the current directory
    memory::utils::memcpy(buf, cwd.c_str(), cwd.size() + 1);

    return buf;
}