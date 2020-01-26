#include <kernel/memory/physical_allocator.h>

extern "C" void _start(const boot_info info) {
    influx::memory::physical_allocator::init(info);

    asm("hlt");
}