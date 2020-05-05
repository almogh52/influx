#include <kernel/kernel.h>
#include <kernel/syscalls/handlers.h>

uint64_t influx::syscalls::handlers::sleep(uint64_t seconds) {
    return kernel::scheduler()->sleep(seconds * 1000) / 1000;
}