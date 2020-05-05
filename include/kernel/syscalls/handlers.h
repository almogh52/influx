#pragma once
#include <kernel/syscalls/stat.h>
#include <kernel/threading/signal.h>
#include <kernel/threading/signal_action.h>
#include <kernel/time/timeval.h>
#include <kernel/vfs/file_info.h>
#include <stddef.h>
#include <stdint.h>

namespace influx {
namespace syscalls {
namespace handlers {
int64_t open(const char *file_name, int flags, int mode);
int64_t close(size_t fd);
int64_t execve(const char *exec_path, const char **argv, const char **envp);
int64_t fstat(size_t fd, influx::syscalls::stat *stat);
int64_t stat(const char *file_path, influx::syscalls::stat *stat);
int64_t lseek(size_t fd, int64_t offset, uint8_t whence);
int64_t read(size_t fd, void *buf, size_t count);
int64_t unlink(const char *file_path);
int64_t write(size_t fd, const void *buf, size_t count);
int64_t gettimeofday(time::timeval *tv, void *tz);
int64_t gethostname(char *name, uint64_t len);
int64_t isatty(size_t fd);
int64_t kill(int64_t pid, int64_t tid, threading::signal sig);
int64_t sigaction(threading::signal signum, const threading::signal_action *act,
                  threading::signal_action *oldact);
int64_t waitpid(int64_t pid, int *wait_status, uint64_t flags);
};  // namespace handlers
};  // namespace syscalls
};  // namespace influx