#pragma once
#include <kernel/structures/string.h>
#include <kernel/structures/unique_vector.h>
#include <stdint.h>

#define MAX_PRIORITY_LEVEL 9
#define DEFAULT_USER_SPACE_PROCESS_PRIORITY 5

namespace influx {
namespace threading {
struct process {
    uint64_t pid;
    uint64_t ppid;

    uint8_t priority;
    bool system;

    uint64_t cr3 __attribute__((packed));

    structures::unique_vector threads;

    structures::string name;
};
};  // namespace threading
};  // namespace influx