#pragma once
#include <kernel/threading/signal.h>
#include <stdint.h>

/* Values for si_code */

/* Codes for SIGILL */
#define ILL_ILLOPC 1 /* illegal opcode */
#define ILL_ILLTRP 2 /* illegal trap */
#define ILL_PRVOPC 3 /* privileged opcode */
#define ILL_ILLOPN 4 /* illegal operand -NOTIMP */
#define ILL_ILLADR 5 /* illegal addressing mode -NOTIMP */
#define ILL_PRVREG 6 /* privileged register -NOTIMP */
#define ILL_COPROC 7 /* coprocessor error -NOTIMP */
#define ILL_BADSTK 8 /* internal stack error -NOTIMP */

/* Codes for SIGFPE */
#define FPE_FLTDIV 1 /* floating point divide by zero */
#define FPE_FLTOVF 2 /* floating point overflow */
#define FPE_FLTUND 3 /* floating point underflow */
#define FPE_FLTRES 4 /* floating point inexact result */
#define FPE_FLTINV 5 /* invalid floating point operation */
#define FPE_FLTSUB 6 /* subscript out of range -NOTIMP */
#define FPE_INTDIV 7 /* integer divide by zero */
#define FPE_INTOVF 8 /* integer overflow */

/* Codes for SIGSEGV */
#define SEGV_MAPERR 1 /* address not mapped to object */
#define SEGV_ACCERR 2 /* invalid permission for mapped object */

/* Codes for SIGBUS */
#define BUS_ADRALN 1 /* Invalid address alignment */
#define BUS_ADRERR 2 /* Nonexistent physical address -NOTIMP */
#define BUS_OBJERR 3 /* Object-specific HW error - NOTIMP */

/* Codes for SIGTRAP */
#define TRAP_BRKPT 1 /* Process breakpoint -NOTIMP */
#define TRAP_TRACE 2 /* Process trace trap -NOTIMP */

/* Codes for SIGCHLD */
#define CLD_EXITED 1    /* child has exited */
#define CLD_KILLED 2    /* terminated abnormally, no core file */
#define CLD_DUMPED 3    /* terminated abnormally, core file */
#define CLD_TRAPPED 4   /* traced child has trapped */
#define CLD_STOPPED 5   /* child has stopped */
#define CLD_CONTINUED 6 /* stopped child has continued */

/* Codes for SIGPOLL */
#define POLL_IN 1  /* Data input available */
#define POLL_OUT 2 /* Output buffers available */
#define POLL_MSG 3 /* Input message available */
#define POLL_ERR 4 /* I/O error */
#define POLL_PRI 5 /* High priority input available */
#define POLL_HUP 6 /* Device disconnected */

namespace influx {
namespace threading {
struct signal_info {
    signal sig;      /* signal number */
    uint64_t error;  /* errno association */
    uint64_t code;   /* signal code */
    uint64_t pid;    /* sending process */
    uint64_t uid;    /* sender's ruid */
    uint64_t status; /* exit value */
    void *addr;      /* faulting instruction */

    /* signal value */
    int value_int;
    void *value_ptr;

    uint64_t pad[8]; /* reserved for future use */
};
};  // namespace threading
};  // namespace influx