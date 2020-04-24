#include <kernel/kernel.h>
#include <kernel/panic.h>

extern "C" void __cxa_pure_virtual() {
    influx::structures::string error_str = "Pure virutal function was called! Panicing..\n";

    influx::kernel::tty_manager()->get_tty(KERNEL_TTY).stderr_write(error_str);

    // Panic
    kpanic();
}