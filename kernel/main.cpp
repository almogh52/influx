#include <kernel/memory/physical_allocator.h>
#include <kernel/memory/virtual_allocator.h>

extern "C" void _start(const boot_info info) {
    influx::memory::physical_allocator::init(info.memory);
    influx::memory::virtual_allocator::init(info.memory);

    asm("hlt");
}