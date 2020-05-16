#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/syscalls/utils.h>

#define SIG_SETMASK 0 /* set mask with sigprocmask() */
#define SIG_BLOCK 1   /* set of signals to block */
#define SIG_UNBLOCK 2 /* set of signals to, well, unblock */

int64_t influx::syscalls::handlers::sigprocmask(uint64_t how,
                                                const influx::threading::signal_mask *set,
                                                influx::threading::signal_mask *oldset) {
    threading::signal_mask old_mask = kernel::scheduler()->get_signal_mask(), new_mask = 0;

    // Validate how parameter
    if (how > SIG_UNBLOCK) {
        return -EINVAL;
    }

    // Verify set pointer is in user memory space
    if (set != nullptr &&
        !utils::is_buffer_in_user_memory(set, sizeof(threading::signal_mask), PROT_READ)) {
        return -EFAULT;
    }

    // Verify old set pointer is in user memory space
    if (oldset != nullptr &&
        !utils::is_buffer_in_user_memory(oldset, sizeof(threading::signal_mask), PROT_WRITE)) {
        return -EFAULT;
    }

    // Set the new signal mask
    if (set != nullptr) {
        // The set of blocked signals is the union of the current set and the set argument.
        if (how == SIG_BLOCK) {
            new_mask = old_mask | *set;
        }
        // The signals in set are removed from the current set of blocked signals.  It is
        // permissible to attempt to unblock a signal which is not blocked.
        else if (how == SIG_UNBLOCK) {
            new_mask = old_mask & ~(*set);
        }
        // The set of blocked signals is set to the argument set.
        else if (how == SIG_SETMASK) {
            new_mask = *set;
        }

        // Set the new signal mask
        kernel::scheduler()->set_signal_mask(new_mask);
    }

    // Set old set
    if (oldset != nullptr) {
        *oldset = old_mask;
    }

    return 0;
}