#pragma once

namespace influx {
namespace syscalls {
enum class syscall {
    exit,
    close,
    execve,
    fork,
    fstat,
    getpid,
    isatty,
    kill,
    link,
    lseek,
    open,
    read,
    sbrk,
    stat,
    times,
    unlink,
    wait,
    write,
    sigaction,
    gettimeofday,
    gethostname,
    sigreturn
};
};
};  // namespace influx