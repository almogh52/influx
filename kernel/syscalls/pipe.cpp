#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::pipe(int *pipefd) {
    uint64_t read_fd = 0, write_fd = 0;

    // Check if the buffer is in the user memory
    if (!utils::is_buffer_in_user_memory(pipefd, sizeof(int) * 2, PROT_WRITE)) {
        return -EFAULT;
    }

    // Create the pipe
    if (!kernel::vfs()->pipe_handler()->create_pipe(&read_fd, &write_fd)) {
        return -EFAULT;
    }

    // Set file descriptors
    pipefd[0] = (int)read_fd;
    pipefd[1] = (int)write_fd;

    return 0;
}