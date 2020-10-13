#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>

int64_t influx::syscalls::handlers::dup(size_t oldfd, size_t newfd) {
    vfs::open_file file;

    uint64_t fd;

	if (oldfd == newfd) {
		return newfd;
	}

    // Verify valid old file descriptor
    if (kernel::scheduler()->get_file_descriptor(oldfd, file) != vfs::error::success) {
        return -EBADF;
    }

	// Duplicate the file descriptor
    fd = kernel::scheduler()->duplicate_file_descriptor(oldfd, newfd == (size_t)-1 ? -1 : newfd);

	return fd;
}