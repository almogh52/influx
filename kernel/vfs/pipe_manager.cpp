#include <kernel/vfs/pipe_manager.h>

#include <kernel/algorithm.h>
#include <kernel/kernel.h>
#include <kernel/memory/heap.h>
#include <kernel/memory/utils.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/unique_lock.h>

influx::vfs::pipe_manager::pipe_manager() : _fs(this) {}

bool influx::vfs::pipe_manager::create_pipe(uint64_t *read_fd, uint64_t *write_fd) {
    pipe *p = new pipe((uint8_t *)kmalloc(PIPE_BUFFER_SIZE), PIPE_BUFFER_SIZE,
                       file_info{.inode = 0,
                                 .type = file_type::fifo,
                                 .size = PIPE_BUFFER_SIZE,
                                 .blocks = PIPE_BUFFER_SIZE / 512,
                                 .permissions = file_permissions{.raw = 3},
                                 .owner_user_id = 0,
                                 .owner_group_id = 0,
                                 .fs_block_size = 0,
                                 .hard_links_count = 1,
                                 .created = kernel::time_manager()->unix_timestamp(),
                                 .modified = kernel::time_manager()->unix_timestamp(),
                                 .accessed = kernel::time_manager()->unix_timestamp()});
    uint64_t pipe_index = 0;

    structures::pair<uint64_t, structures::reference_wrapper<vnode>> vn(
        0, kernel::vfs()->_vnodes.empty_item());

    // Insert the pipe
    threading::unique_lock lk(_pipes_mutex);
    pipe_index = _pipes.insert_unique(p);
    lk.unlock();

    // Create the vnode for the pipe
    threading::unique_lock vnodes_lk(kernel::vfs()->_vnodes_mutex);
    if (kernel::vfs()->create_vnode_for_file((filesystem *)&_fs, new uint64_t(pipe_index), p->file,
                                             vn) != error::success) {
        return false;
    }
    vnodes_lk.unlock();

    // Create read and write file descriptors
    *read_fd = kernel::scheduler()->add_file_descriptor(open_file{.vnode_index = vn.first,
                                                                  .position = 0,
                                                                  .flags = open_flags::read,
                                                                  .amount_of_file_descriptors = 1});
    *write_fd =
        kernel::scheduler()->add_file_descriptor(open_file{.vnode_index = vn.first,
                                                           .position = 0,
                                                           .flags = open_flags::write,
                                                           .amount_of_file_descriptors = 1});

    return true;
}

bool influx::vfs::pipe_manager::close_pipe(uint64_t pipe_index) {
    threading::lock_guard lk(_pipes_mutex);

    // Check if the pipe exists
    if (_pipes.count(pipe_index)) {
        return false;
    }

    // Free and erase pipe
    delete (uint8_t *)_pipes[pipe_index]->buffer.buffer();
    delete _pipes[pipe_index];
    _pipes.erase(pipe_index);

    return true;
}

uint64_t influx::vfs::pipe_manager::read(uint64_t pipe_index, void *buf, size_t count) {
    pipe *p = get_pipe(pipe_index);
    threading::unique_lock pipe_lk(p->mutex);

    uint64_t amount_read = 0;

    // If the pipe is empty
    if (p->buffer.current_size() == 0) {
        // If the read was interrupted
        if (p->amount_of_write_file_descriptors == 0 || !p->read_cv.wait_interruptible(pipe_lk)) {
            return 0;
        }
    }

    // Copy from the buffer
    amount_read = algorithm::min<uint64_t>(count, p->buffer.current_size());
    p->buffer.pop((uint8_t *)buf, amount_read);

    // Notify write threads
    if (amount_read > 0) {
        p->write_cv.notify_one();
    }

    // Update last accessed time
    p->file.accessed = kernel::time_manager()->unix_timestamp();

    return amount_read;
}

uint64_t influx::vfs::pipe_manager::write(uint64_t pipe_index, const void *buf, size_t count) {
    pipe *p = get_pipe(pipe_index);
    threading::unique_lock pipe_lk(p->mutex);

    uint64_t current_write = 0, amount_write = 0;

    // Check if the pipe is closed
    if (p->amount_of_read_file_descriptors == 0) {
        return (uint64_t)-1;
    }

    // While we didn't write the wanted amount
    while (amount_write < count) {
        // Check if the pipe is closed
        if (p->amount_of_read_file_descriptors == 0) {
            return (uint64_t)-1;
        }

        // If the pipe is full
        if (p->buffer.current_size() == p->buffer.max_size()) {
            // If the write was interrupted
            if (!p->write_cv.wait_interruptible(pipe_lk)) {
                break;
            }
        }

        // Copy from the buffer
        current_write =
            algorithm::min<uint64_t>(count, p->buffer.max_size() - p->buffer.current_size());
        p->buffer.push((const uint8_t *)buf, current_write);
        amount_write += current_write;

        // Notify read threads
        if (current_write > 0) {
            p->read_cv.notify_one();
        }
    }

    // Update last accessed time and last modified time
    p->file.accessed = kernel::time_manager()->unix_timestamp();
    p->file.modified = kernel::time_manager()->unix_timestamp();

    return amount_write;
}

bool influx::vfs::pipe_manager::pipe_exists(uint64_t pipe_index) {
    threading::lock_guard pipes_lk(_pipes_mutex);
    return _pipes.count(pipe_index) != 0;
}

influx::vfs::file_info influx::vfs::pipe_manager::get_pipe_file_info(uint64_t pipe_index) {
    pipe *p = get_pipe(pipe_index);
    threading::lock_guard pipe_lk(p->mutex);

    return p->file;
}

influx::vfs::pipe *influx::vfs::pipe_manager::get_pipe(uint64_t pipe_index) {
    threading::lock_guard pipes_lk(_pipes_mutex);
    kassert(_pipes.count(pipe_index));

    return _pipes[pipe_index];
}