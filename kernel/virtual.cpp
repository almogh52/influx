#include <kernel/console/console.h>
#include <kernel/panic.h>

extern "C" void __cxa_pure_virtual() {
    influx::console::print(influx::output_stream::stderr,
                           "Pure virutal function was called! Panicing..\n");

    // Panic
    kpanic();
}