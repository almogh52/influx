#include <kernel/panic.h>

#include <kernel/console/console.h>
#include <kernel/format.h>
#include <kernel/logger.h>
#include <stdarg.h>

void kpanic(const char* fmt, ...) {
    influx::logger log("Kernel Error", influx::console_color::red);

    va_list args;
    va_start(args, fmt);

    // Verify that the current console isn't null
    if (influx::console::get_console() != nullptr) {
        if (fmt) {
            log.vlog(fmt, args);
        }
        log("Panicing..\n");
    }

    va_end(args);

    // Disable interrupts and halt
    __asm__ __volatile__("cli; hlt;");
}