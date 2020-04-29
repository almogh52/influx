#pragma once
#include <kernel/segment.h>
#include <kernel/structures/string.h>
#include <kernel/structures/unique_hash_map.h>
#include <kernel/structures/unique_vector.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/signal.h>
#include <kernel/threading/signal_action.h>
#include <kernel/vfs/open_file.h>
#include <memory/paging.h>
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
    pml4e_t* pml4t;

    uint64_t program_break_start;
    uint64_t program_break_end;

    structures::unique_vector threads;
    structures::vector<uint64_t> child_processes;

    structures::unique_hash_map<vfs::open_file> file_descriptors;
    structures::string name;

    structures::vector<segment> segments;

    structures::vector<signal_action> signal_dispositions;
    structures::vector<signal_info> pending_std_signals;

    uint64_t tty;
    uint8_t exit_code;

    bool terminated;
    bool new_exec_process;

    inline bool operator==(const process& p) const { return pid == p.pid && name == p.name; };
    inline bool operator!=(const process& p) const { return !(*this == p); }
};
};  // namespace threading
};  // namespace influx