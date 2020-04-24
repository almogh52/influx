#include <kernel/kernel.h>

#include <kernel/assert.h>
#include <kernel/console/console.h>
#include <kernel/console/early_console.h>
#include <kernel/console/gfx_console.h>
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

    // Init tty manager
    _tty_manager = new tty::tty_manager();
    _tty_manager->init();

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

    // Init GFX console
    log("Loading GFX console..\n");
    console::set_console(new gfx_console(info.framebuffer));
    log("GFX Console loaded.\n");

    // Init scheduler
    log("Loading scheduler..\n");
    _scheduler = new threading::scheduler(info.tss_address);
    log("Scheduler loaded.\n");

    // Init syscall manager
    log("Loading syscall manager..\n");
    _syscall_manager = new syscalls::syscall_manager();
    log("Syscall manager loaded.\n");

    // Init VFS
    drivers::ata::ata *ata = (drivers::ata::ata *)_driver_manager->get_driver("ATA");
    _vfs = new vfs::vfs();
    if (!_vfs->mount(vfs::fs_type::ext2, vfs::path("/"),
                     drivers::ata::drive_slice(ata, ata->drives()[0], 0))) {
        kpanic("Unable to mount main drive!\n");
    }

    // Kill this task since it's no necessary
    log("Kernel initialization complete.\n");
    _scheduler->kill_current_task();
}