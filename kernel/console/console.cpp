#include <kernel/console/console.h>

#include <kernel/assert.h>
#include <kernel/format.h>
#include <kernel/kernel.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/unique_lock.h>
#include <stdarg.h>

influx::console *influx::console::get_console() {
    threading::lock_guard lk(_console_mutex);
    return _console;
}

influx::console *influx::console::set_console(influx::console *console) {
    threading::unique_lock lk(_console_mutex);

    influx::console *old_console = _console;

    // Try to load the new console
    if (_console) {
        lk.unlock();
        _log("Loading new console at address %p.\n", console);
        lk.lock();
    }
    if (!console->load()) {
        lk.unlock();
        if (_console) {
            _log("Console load failed!\n");
        }
        return console;
    }

    // Set the new console
    _console = console;

    // Reload TTY
    lk.unlock();
    kernel::tty_manager()->reload();

    return old_console;
}