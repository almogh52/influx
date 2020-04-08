#pragma once
#include <stdint.h>

#define AMOUNT_OF_SSE_REGISTERS 16

typedef double sse_128 __attribute__((vector_size(16)));

namespace influx {
namespace threading {
struct regs {
    sse_128 xmm[AMOUNT_OF_SSE_REGISTERS];
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rbp;
} __attribute__((packed));
};  // namespace threading
};  // namespace influx