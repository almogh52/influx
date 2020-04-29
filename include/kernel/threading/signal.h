#pragma once
#include <stdint.h>

#define SIGHUP 1       /* hangup */
#define SIGINT 2       /* interrupt */
#define SIGQUIT 3      /* quit */
#define SIGILL 4       /* illegal instruction (not reset when caught) */
#define SIGTRAP 5      /* trace trap (not reset when caught) */
#define SIGABRT 6      /* abort() */
#define SIGIOT SIGABRT /* compatibility */
#define SIGBUS 7       /* bus error */
#define SIGFPE 8       /* floating point exception */
#define SIGKILL 9      /* kill (cannot be caught or ignored) */
#define SIGUSR1 10     /* user defined signal 1 */
#define SIGSEGV 11     /* segmentation violation */
#define SIGUSR2 12     /* user defined signal 2 */
#define SIGPIPE 13     /* write on a pipe with no one to read it */
#define SIGALRM 14     /* alarm clock */
#define SIGTERM 15     /* software termination signal from kill */
#define SIGSTKFLT 16   /* stack fault on coprocessor (unused) */
#define SIGCHLD 17     /* child stopped or terminated */
#define SIGCLD SIGCHLD /* compatibility */
#define SIGCONT 18     /* continue a stopped process */
#define SIGSTOP 19     /* stop process */
#define SIGTSTP 20     /* stop typed at terminal */
#define SIGTTIN 21     /* to readers pgrp upon background tty read */
#define SIGTTOU 22     /* like TTIN for output if (tp->t_local&LTOSTOP) */
#define SIGURG 23      /* urgent condition on IO channel */
#define SIGXCPU 24     /* exceeded CPU time limit */
#define SIGXFSZ 25     /* exceeded file size limit */
#define SIGVTALRM 26   /* virtual time alarm */
#define SIGPROF 27     /* profiling time alarm */
#define SIGWINCH 28    /* window size changes */
#define SIGIO 29       /* I/O now possible */
#define SIGPOLL SIGIO  /* compatibility */
#define SIGPWR 30      /* power failure */
#define SIGSYS 31      /* bad argument to system call */

#define SIGINVL (uint64_t) - 1
#define SIGSTD_N 32

namespace influx {
namespace threading {
typedef uint64_t signal;
typedef uint64_t signal_mask;
};  // namespace threading
};  // namespace influx