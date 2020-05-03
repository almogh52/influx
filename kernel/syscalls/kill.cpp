#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>
#include <kernel/threading/signal_info.h>

int64_t influx::syscalls::handlers::kill(int64_t pid, int64_t tid, influx::threading::signal sig) {
    threading::signal_info sig_info{.sig = sig,
                                    .error = 0,
                                    .code = 0,
                                    .pid = kernel::scheduler()->get_current_process_id(),
                                    .uid = 0,
                                    .status = 0,
                                    .addr = nullptr,
                                    .value_int = 0,
                                    .value_ptr = nullptr,
                                    .pad = {0}};

    // Check valid signal
    if (sig >= SIGSTD_N) {
        return -EINVAL;
    }

    // Try to send the signal
    if (!kernel::scheduler()->send_signal(pid, tid, sig_info)) {
        return -ESRCH;
    }

    return 0;
}