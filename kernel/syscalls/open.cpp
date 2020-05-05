#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/stat.h>
#include <kernel/syscalls/utils.h>

#define O_RDONLY 0x0000 /* open for reading only */
#define O_WRONLY 0x0001 /* open for writing only */
#define O_RDWR 0x0002   /* open for reading and writing */
#define O_APPEND 0x0008 /* set append mode */
#define O_CREAT 0x0200  /* create if nonexistant */
#define O_DIRECTORY 0x100000

influx::vfs::open_flags convert_flags(int flags) {
    influx::vfs::open_flags f = (influx::vfs::open_flags)0;

    if (flags & O_WRONLY) {
        f = (influx::vfs::open_flags)(f | influx::vfs::open_flags::write);
    } else if (flags & O_RDWR) {
        f = (influx::vfs::open_flags)(f | influx::vfs::open_flags::read |
                                      influx::vfs::open_flags::write);
    } else {
        f = (influx::vfs::open_flags)(f | influx::vfs::open_flags::read);
    }

    if (flags & O_APPEND) {
        f = (influx::vfs::open_flags)(f | influx::vfs::open_flags::append);
    }

    if (flags & O_CREAT) {
        f = (influx::vfs::open_flags)(f | influx::vfs::open_flags::create);
    }

    if (flags & O_DIRECTORY) {
        f = (influx::vfs::open_flags)(f | influx::vfs::open_flags::directory);
    }

    return f;
}

influx::vfs::file_permissions convert_permissions(int mode) {
    influx::vfs::file_permissions permissions;

    permissions.read = (mode & S_IRUSR) != 0;
    permissions.write = (mode & S_IWUSR) != 0;
    permissions.execute = (mode & S_IXUSR) != 0;

    return permissions;
}

int64_t influx::syscalls::handlers::open(const char *file_name, int flags, int mode) {
    vfs::open_flags open_flags = convert_flags(flags);
    vfs::file_permissions permissions = convert_permissions(mode);

    int64_t fd = 0;

    // Verify mode
    if ((flags & O_CREAT) > 0 && mode & ~(S_IRWXU | S_IRWXG | S_IRWXO)) {
        return -EINVAL;
    }

    // Verify file name buffer
    if (!utils::is_string_in_user_memory(file_name)) {
        return -EFAULT;
    }

    // Try to open the file
    fd = kernel::vfs()->open(file_name, open_flags, permissions);

    // Handle errors
    if (fd < 0) {
        return utils::convert_vfs_error((vfs::error)fd);
    }

    return fd;
}