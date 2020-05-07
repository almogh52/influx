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
    waitpid,
    write,
    sigaction,
    gettimeofday,
    gethostname,
    sigreturn,
    sleep,
    getdents,
    chdir,
    getcwd,
    mkdir,
    ttyname,
    getppid,
    getuid,
    geteuid,
    dup
};
};
};  // namespace influx