#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>

int64_t influx::syscalls::handlers::isatty(size_t fd) {
    vfs::filesystem *fs = kernel::vfs()->get_filesystem(fd);

    return fs == nullptr ? -EBADF : (fs->name() == "TTY" ? 1 : -ENOTTY);
}