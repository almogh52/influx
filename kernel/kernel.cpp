#include <kernel/kernel.h>

#include <kernel/assert.h>
#include <kernel/console/bga_console.h>
#include <kernel/console/console.h>
#include <kernel/console/early_console.h>
#include <kernel/icxxabi.h>
#include <kernel/logger.h>
#include <kernel/memory/physical_allocator.h>
#include <kernel/memory/virtual_allocator.h>

extern "C" void _init();

void influx::kernel::start(const boot_info info) {
    early_kmain(info);
    kmain(info);
}

void influx::kernel::early_kmain(const boot_info info) {
    // Init memory manager
    memory::physical_allocator::init(info.memory);
    memory::virtual_allocator::init(info.memory);

    // Init global constructors
    _init();

    // Init early console
    console::set_console(new early_console());
}

void influx::kernel::kmain(const boot_info info) {
    logger log("Kernel");

    log("%s Kernel, v%s\n", OS_NAME, KERNEL_VERSION);
    log("Made by Almog Hamdani.\n\n\n");

    log("Finished initializing memory manager and console.\n");

    // Init interrupt manager
    log("Loading interrupt manager..\n");
    _interrupt_manager = new interrupts::interrupt_manager();

    // Init driver manager and load drivers
    log("Loading driver manager..\n");
    _driver_manager = new drivers::driver_manager();
    _driver_manager->load_drivers();

    // Init time manager
    _time_manager = new time::time_manager();

    // Init BGA console
    log("Loading BGA console..\n");
    console::set_console(new bga_console());
    log("BGA Console loaded.\n");

    kassert(0 == 1);

    // Listen to interrupts and halt
    while (true) {
        __asm__ __volatile__("hlt");
    }

    // Destruct all global objects
    __cxa_finalize(nullptr);
}