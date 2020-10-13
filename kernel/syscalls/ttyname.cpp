#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::ttyname(size_t fd, char *buf, size_t buflen) {
    int64_t ret;

    structures::string tty_name;

    // Check valid buffer
    if (!utils::is_buffer_in_user_memory(buf, buflen, PROT_WRITE)) {
        return -EFAULT;
    }

    // If the file descriptor is not a TTY
    if ((ret = handlers::isatty(fd)) != 1) {
        return ret;
    }

    // Get the vnode index
    if ((ret = kernel::vfs()->get_vnode_index(fd)) == -1) {
        return -EBADF;
    }

    // Get the TTY name
    tty_name = kernel::tty_manager()->get_tty_name(ret);

    // Check if the tty name can fit in the buffer
    if (tty_name.size() + 1 > buflen) {
        return -ERANGE;
    }

    // Copy the string
    memory::utils::memcpy(buf, tty_name.c_str(), tty_name.size() + 1);

    return 0;
}