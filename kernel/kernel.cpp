#include <kernel/kernel.h>

#include <kernel/console/console.h>
#include <kernel/console/early_console.h>
#include <kernel/logger.h>
#include <kernel/memory/physical_allocator.h>
#include <kernel/memory/virtual_allocator.h>

void influx::kernel::start(const boot_info info) {
    early_kmain(info);
    kmain(info);
}

void influx::kernel::early_kmain(const boot_info info) {
    // Init memory manager
    memory::physical_allocator::init(info.memory);
    memory::virtual_allocator::init(info.memory);

    // Init early console
    console::set_console(new early_console());
}

void influx::kernel::kmain(const boot_info info) {
    logger log("Kernel");

    log("%s Kernel, v%s\n", OS_NAME, KERNEL_VERSION);
    log("Made by Almog Hamdani.\n\n\n");

    log("Finished initializing memory manager and console.\n");

    asm("hlt");
}