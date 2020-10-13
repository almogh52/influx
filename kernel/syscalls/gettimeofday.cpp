#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::gettimeofday(influx::time::timeval *tv, void *tz) {
    // No support for timezone
    if (tz != nullptr) {
        return -EFAULT;
    }

    // Check if the timeval struct is in the user memory
    if (!utils::is_buffer_in_user_memory(tv, sizeof(tv), PROT_WRITE)) {
        return -EFAULT;
    }

    // Get the timeval
    *tv = kernel::time_manager()->get_timeval();

    return 0;
}