#pragma once
#include <kernel/threading/signal_action.h>
#include <kernel/threading/signal_info.h>

#define SIG_DFL 0
#define SIG_IGN 1

#define SA_ONSTACK 0x0001   /* take signal on alternate signal stack */
#define SA_RESTART 0x0002   /* restart system calls on signal return */
#define SA_RESETHAND 0x0004 /* reset to SIG_DFL when taking signal */
#define SA_NOCLDSTOP 0x0008 /* do not generate SIGCHLD on child stop */
#define SA_NODEFER 0x0010   /* don't mask the signal we're delivering */
#define SA_NOCLDWAIT 0x0020 /* don't keep zombies around */
#define SA_SIGINFO 0x0040   /* signal handler with SA_SIGINFO args */

namespace influx {
namespace threading {
union signal_handler {
    uint64_t raw;
    void (*handler_func)(int);
    void (*action_func)(int, signal_info *, void *);
};

struct signal_action {
    signal_handler handler;
    signal_mask mask;
    int flags;
    void (*restorer)(void);
};
};  // namespace threading
};  // namespace influx