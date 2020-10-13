#include <kernel/vfs/pipe_filesystem.h>

#include <kernel/kernel.h>
#include <kernel/vfs/pipe_manager.h>

influx::vfs::pipe_filesystem::pipe_filesystem(influx::vfs::pipe_manager *manager)
    : filesystem("PIPE", drivers::ata::drive_slice()), _manager(manager) {}

influx::vfs::error influx::vfs::pipe_filesystem::read(void *fs_file_info, char *buffer,
                                                      size_t count, size_t offset,
                                                      size_t &amount_read) {
    uint64_t pipe_index = *(uint64_t *)fs_file_info;

    // Verify that the pipe exists
    if (!_manager->pipe_exists(pipe_index)) {
        return error::invalid_file;
    }

    // Read from the pipe
    amount_read = _manager->read(pipe_index, buffer, count);
    if (amount_read == 0 && kernel::scheduler()->interrupted()) {
        return error::interrupted;
    }

    return error::success;
}

influx::vfs::error influx::vfs::pipe_filesystem::write(void *fs_file_info, const char *buffer,
                                                       size_t count, size_t offset,
                                                       size_t &amount_written) {
    uint64_t pipe_index = *(uint64_t *)fs_file_info;

    // Verify that the pipe exists
    if (!_manager->pipe_exists(pipe_index)) {
        return error::invalid_file;
    }

    // Write to the pipe
    amount_written = _manager->write(pipe_index, buffer, count);
    if (amount_written == 0 && kernel::scheduler()->interrupted()) {
        return error::interrupted;
    } else if (amount_written == (uint64_t)-1) {
        // Raise SIGPIPE
        kernel::scheduler()->send_signal(
            kernel::scheduler()->get_current_process_id(), -1,
            threading::signal_info{.sig = SIGPIPE,
                                   .error = 0,
                                   .code = 0,
                                   .pid = kernel::scheduler()->get_current_process_id(),
                                   .uid = 0,
                                   .status = 0,
                                   .addr = nullptr,
                                   .value_int = 0,
                                   .value_ptr = nullptr,
                                   .pad = {0}});

        return error::pipe_closed;
    }

    return error::success;
}

influx::vfs::error influx::vfs::pipe_filesystem::get_file_info(void *fs_file_info,
                                                               influx::vfs::file_info &file) {
    uint64_t pipe_index = *(uint64_t *)fs_file_info;

    // Verify that the pipe exists
    if (!_manager->pipe_exists(pipe_index)) {
        return error::invalid_file;
    }

    // Read file info
    file = _manager->get_pipe_file_info(pipe_index);

    return error::success;
}

bool influx::vfs::pipe_filesystem::compare_fs_file_data(void *fs_file_data_1,
                                                        void *fs_file_data_2) {
    return *(uint64_t *)fs_file_data_1 == *(uint64_t *)fs_file_data_2;
}

void influx::vfs::pipe_filesystem::duplicate_open_file(const influx::vfs::open_file &file,
                                                       void *fs_file_info) {
    uint64_t pipe_index = *(uint64_t *)fs_file_info;

    pipe *p = nullptr;

    // Verify that the pipe exists
    if (!_manager->pipe_exists(pipe_index)) {
        return;
    }

    // Get the pipe and lock it's mutex
    p = _manager->get_pipe(pipe_index);
    threading::lock_guard lk(p->mutex);

    // If the file descriptor is a read file descriptor
    if (file.flags == open_flags::read) {
        p->amount_of_read_file_descriptors++;
    } else {
        p->amount_of_write_file_descriptors++;
    }
}

void influx::vfs::pipe_filesystem::close_open_file(const influx::vfs::open_file &file,
                                                   void *fs_file_info) {
    uint64_t pipe_index = *(uint64_t *)fs_file_info;

    pipe *p = nullptr;

    // Verify that the pipe exists
    if (!_manager->pipe_exists(pipe_index)) {
        return;
    }

    // Get the pipe and lock it's mutex
    p = _manager->get_pipe(pipe_index);
    threading::lock_guard lk(p->mutex);

    // If the file descriptor is a read file descriptor
    if (file.flags == open_flags::read) {
        p->amount_of_read_file_descriptors--;
    } else {
        p->amount_of_write_file_descriptors--;
    }
}