#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

int64_t influx::syscalls::handlers::sigaction(influx::threading::signal signum,
                                              const influx::threading::signal_action *act,
                                              influx::threading::signal_action *oldact) {
    // Check valid signal
    if (signum < 1 || signum >= SIGSTD_N || signum == SIGKILL || signum == SIGSTOP) {
        return -EINVAL;
    }

    // Check valid action struct
    if (act != nullptr &&
        !utils::is_buffer_in_user_memory(act, sizeof(threading::signal_action), PROT_READ)) {
        return -EFAULT;
    }

    // Check valid old action struct
    if (oldact != nullptr &&
        !utils::is_buffer_in_user_memory(oldact, sizeof(threading::signal_action), PROT_WRITE)) {
        return -EFAULT;
    }

    // Get old action
    if (oldact != nullptr) {
        *oldact = kernel::scheduler()->get_signal_action(signum);
    }

    // Set new action
    if (act != nullptr) {
        kernel::scheduler()->set_signal_action(signum, *act);
    }

    return 0;
}