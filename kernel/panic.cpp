#include <kernel/panic.h>

#include <kernel/console/console.h>

void kpanic() {
    influx::console::print("Panicing..\n");

    // Disable interrupts and halt
    __asm__ __volatile__("cli; hlt;");
}