#pragma once
#include <kernel/drivers/driver_manager.h>
#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/syscalls/syscall_manager.h>
#include <kernel/threading/scheduler.h>
#include <kernel/time/time_manager.h>
#include <kernel/tty/tty_manager.h>
#include <kernel/vfs/vfs.h>
#include <sys/boot_info.h>

#define OS_NAME "Influx"
#define KERNEL_VERSION "0.3.0"

namespace influx {
class kernel {
   public:
    static void start(const boot_info info);

    inline static interrupts::interrupt_manager *interrupt_manager() { return _interrupt_manager; }
    inline static drivers::driver_manager *driver_manager() { return _driver_manager; }
    inline static time::time_manager *time_manager() { return _time_manager; }
    inline static threading::scheduler *scheduler() { return _scheduler; }
    inline static syscalls::syscall_manager *syscall_manager() { return _syscall_manager; }
    inline static vfs::vfs *vfs() { return _vfs; }
    inline static tty::tty_manager *tty_manager() { return _tty_manager; }

   private:
    inline static interrupts::interrupt_manager *_interrupt_manager = nullptr;
    inline static drivers::driver_manager *_driver_manager = nullptr;
    inline static time::time_manager *_time_manager = nullptr;
    inline static threading::scheduler *_scheduler = nullptr;
    inline static syscalls::syscall_manager *_syscall_manager = nullptr;
    inline static vfs::vfs *_vfs = nullptr;
    inline static tty::tty_manager *_tty_manager = nullptr;

    static void early_kmain(const boot_info info);
    static void kmain(const boot_info info);
};
};  // namespace influx