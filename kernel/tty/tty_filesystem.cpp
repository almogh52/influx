#include <kernel/tty/tty_filesystem.h>

#include <kernel/kernel.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/tty/tty_manager.h>

influx::tty::tty_filesystem::tty_filesystem()
    : filesystem("TTY", drivers::ata::drive_slice()),
      _creation_time(kernel::time_manager()->unix_timestamp()) {
    // Set last access time for each TTY
    for (uint64_t i = 0; i < AMOUNT_OF_TTYS; i++) {
        _tty_access_times.push_back(_creation_time);
    }
}

influx::vfs::error influx::tty::tty_filesystem::read(void *fs_file_info, char *buffer, size_t count,
                                                     size_t offset, size_t &amount_read) {
    uint64_t *tty = (uint64_t *)fs_file_info;

    // Check for invalid tty
    if (*tty < 1 || *tty > AMOUNT_OF_TTYS) {
        return vfs::error::invalid_file;
    }

    // Read from the tty
    amount_read = kernel::tty_manager()->get_tty(*tty).stdin_read(buffer, count);
    if (amount_read == 0 && kernel::scheduler()->interrupted()) {
        return vfs::error::interrupted;
    }

    // Update last access time
    threading::lock_guard lk(_tty_access_times_mutex);
    _tty_access_times[*tty - 1] = kernel::time_manager()->unix_timestamp();

    return vfs::error::success;
}

influx::vfs::error influx::tty::tty_filesystem::write(void *fs_file_info, const char *buffer,
                                                      size_t count, size_t offset,
                                                      size_t &amount_written) {
    structures::string str(buffer, count);
    uint64_t *tty = (uint64_t *)fs_file_info;

    // Check for invalid tty
    if (*tty < 1 || *tty > AMOUNT_OF_TTYS) {
        return vfs::error::invalid_file;
    }

    // Write to the tty
    kernel::tty_manager()->get_tty(*tty).stdout_write(str);
    amount_written = count;

    // Update last access time
    threading::lock_guard lk(_tty_access_times_mutex);
    _tty_access_times[*tty - 1] = kernel::time_manager()->unix_timestamp();

    return vfs::error::success;
}

influx::vfs::error influx::tty::tty_filesystem::get_file_info(void *fs_file_info,
                                                              influx::vfs::file_info &file) {
    threading::lock_guard lk(_tty_access_times_mutex);
    uint64_t *tty = (uint64_t *)fs_file_info;

    // Check for invalid tty
    if (*tty < 1 || *tty > AMOUNT_OF_TTYS) {
        return vfs::error::invalid_file;
    }

    file.inode = *tty;
    file.type = vfs::file_type::character_device;
    file.size = UINT64_MAX;
    file.blocks = 0;
    file.permissions.read = true;
    file.permissions.write = true;
    file.permissions.execute = false;
    file.owner_user_id = 0;
    file.owner_group_id = 0;
    file.fs_block_size = 0;
    file.hard_links_count = 1;
    file.created = _creation_time;
    file.accessed = _tty_access_times[*tty - 1];
    file.modified = _tty_access_times[*tty - 1];

    return vfs::error::success;
}

bool influx::tty::tty_filesystem::compare_fs_file_data(void *fs_file_data_1, void *fs_file_data_2) {
    return *(uint64_t *)fs_file_data_1 == *(uint64_t *)fs_file_data_2;
}