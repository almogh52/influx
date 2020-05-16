#pragma once
#include <kernel/structures/unique_hash_map.h>
#include <kernel/threading/mutex.h>
#include <kernel/vfs/pipe.h>
#include <kernel/vfs/pipe_filesystem.h>
#include <stdint.h>

#define PIPE_BUFFER_SIZE (1 << 16)

namespace influx {
namespace vfs {
class pipe_manager {
   public:
    pipe_manager();

    bool create_pipe(uint64_t* read_fd, uint64_t* write_fd);

   private:
    pipe_filesystem _fs;

    threading::mutex _pipes_mutex;
    structures::unique_hash_map<pipe*> _pipes;

    file_info get_pipe_file_info(uint64_t pipe_index);
    bool pipe_exists(uint64_t pipe_index);
    bool close_pipe(uint64_t pipe_index);

    uint64_t read(uint64_t pipe_index, void* buf, size_t count);
    uint64_t write(uint64_t pipe_index, const void* buf, size_t count);

    pipe *get_pipe(uint64_t pipe_index);

    friend class pipe_filesystem;
};
};  // namespace vfs
};  // namespace influx