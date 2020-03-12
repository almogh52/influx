#pragma once
#include <kernel/structures/node.h>
#include <kernel/threading/regs.h>
#include <stdint.h>

namespace influx {
namespace threading {
enum class thread_state { ready, running, blocked, sleeping, waiting_for_child, killed };

struct thread {
    uint64_t tid;
    uint64_t pid;

    regs* context;
    void* kernel_stack;

    thread_state state;
    uint64_t quantum;
};

typedef structures::node<thread> tcb;
};  // namespace threading
};  // namespace influx