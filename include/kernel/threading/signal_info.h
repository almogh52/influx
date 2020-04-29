#pragma once
#include <kernel/threading/signal.h>
#include <stdint.h>

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