#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

#define SEEK_SET 0 /* set file offset to offset */
#define SEEK_CUR 1 /* set file offset to current plus offset */
#define SEEK_END 2 /* set file offset to EOF plus offset */

influx::vfs::seek_type convert_whence_to_seek_type(uint8_t whence) {
    switch (whence) {
        case SEEK_SET:
            return influx::vfs::seek_type::set;

        case SEEK_CUR:
            return influx::vfs::seek_type::current;

        case SEEK_END:
            return influx::vfs::seek_type::end;
    }

    return influx::vfs::seek_type::set;
}

int64_t influx::syscalls::handlers::lseek(size_t fd, int64_t offset, uint8_t whence) {
    int64_t pos = 0;

    vfs::seek_type seek = convert_whence_to_seek_type(whence);

    // Verify valid whence
    if (whence > SEEK_END) {
        return -EINVAL;
    }

    // Verify valid file descriptor
    if (fd < 3) {
        return -EBADF;
    }

    // Change the file position
    pos = kernel::vfs()->seek(fd, offset, seek);

    // Handle errors
    if (pos < 0) {
        return utils::convert_vfs_error((vfs::error)pos);
    }

    return pos;
}