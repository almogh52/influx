#pragma once
#include <kernel/interrupts/interrupt_regs.h>
#include <kernel/structures/node.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/regs.h>
#include <kernel/threading/signal.h>
#include <kernel/threading/signal_info.h>
#include <stdint.h>

namespace influx {
namespace threading {
enum class thread_state { ready, running, blocked, sleeping, waiting_for_child, killed };

struct thread {
    uint64_t tid;
    uint64_t pid;

    regs* context;

    void* kernel_stack;
    void* user_stack;
    uint64_t args_size;

    thread_state state;
    uint64_t quantum;
    uint64_t sleep_quantum;

    int64_t child_wait_pid;

    bool signal_interruptible;
    bool signal_interrupted;

    structures::vector<signal_info> sig_queue;
    signal current_sig;
    signal_mask sig_mask;

    interrupts::regs old_interrupt_regs;
    signal_mask old_sig_mask;
};

typedef structures::node<thread> tcb;
};  // namespace threading
};  // namespace influx