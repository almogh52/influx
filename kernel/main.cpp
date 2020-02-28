#include <kernel/kernel.h>

extern "C" void _start(const boot_info info) {
    influx::kernel::start(info);
}