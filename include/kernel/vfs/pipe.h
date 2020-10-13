#pragma once
#include <kernel/structures/fifo.h>
#include <kernel/threading/condition_variable.h>
#include <kernel/threading/mutex.h>
#include <kernel/vfs/file_info.h>
#include <stdint.h>

namespace influx {
namespace vfs {
struct pipe {
    inline pipe(uint8_t *raw_buffer, uint64_t size, file_info pipe_file)
        : buffer(raw_buffer, size),
          amount_of_read_file_descriptors(1),
          amount_of_write_file_descriptors(1),
          file(pipe_file) {}

    structures::fifo buffer;

    uint64_t amount_of_read_file_descriptors;
    uint64_t amount_of_write_file_descriptors;

    file_info file;

    threading::mutex mutex;

    threading::condition_variable read_cv;
    threading::condition_variable write_cv;
};
};  // namespace vfs
};  // namespace influx