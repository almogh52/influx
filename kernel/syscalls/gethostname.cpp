#include <kernel/algorithm.h>
#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

constexpr char hostname[] = "INFLUX_PC";

int64_t influx::syscalls::handlers::gethostname(char *name, uint64_t len) {
    uint64_t dest_string_len = algorithm::min<uint64_t>(sizeof(hostname) - 1, len - 1);

    // Check if the buffer is in the user memory
    if (!utils::is_buffer_in_user_memory(name, len, PROT_WRITE)) {
        return -EFAULT;
    }

    // Copy hostname
    memory::utils::memcpy(name, hostname, dest_string_len);
    name[dest_string_len] = '\0';

    return 0;
}