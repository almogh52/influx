#include <kernel/kernel.h>
#include <kernel/console/console.h>
#include <kernel/console/early_console.h>
#include <kernel/memory/physical_allocator.h>
#include <kernel/memory/virtual_allocator.h>

extern "C" void _start(const boot_info info) {
    // Init memory manager
    influx::memory::physical_allocator::init(info.memory);
    influx::memory::virtual_allocator::init(info.memory);

    // Init early console
    influx::console::set_console(new influx::early_console());
    influx::console::print("%s Kernel, v%s\n", OS_NAME, KERNEL_VERSION);
    influx::console::print("Made by Almog Hamdani.\n\n\n");
    influx::console::print("Finished initializing memory manager and console.\n");

    asm("hlt");
}