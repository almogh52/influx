#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

#define WNOHANG 1
#define WUNTRACED 2

int64_t influx::syscalls::handlers::waitpid(int64_t pid, int *wait_status, uint64_t flags) {
    int64_t ret = 0;
    uint16_t kernel_wstatus = 0;

    // Check valid wait status variable
    if (wait_status != nullptr &&
        !utils::is_buffer_in_user_memory(wait_status, sizeof(int), PROT_WRITE)) {
        return -EFAULT;
    }

    // Wait for the child
    ret = kernel::scheduler()->wait_for_child(
        pid, wait_status == nullptr ? nullptr : &kernel_wstatus, (flags & WNOHANG) > 0);
    if (ret < 0) {
        return kernel::scheduler()->interrupted() ? -EINTR : -ECHILD;
    }

    // Set wait status
    if (wait_status != nullptr) {
        *wait_status = kernel_wstatus;
    }

    return ret;
}