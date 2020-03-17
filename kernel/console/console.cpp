#include <kernel/console/console.h>

#include <kernel/assert.h>
#include <kernel/format.h>
#include <kernel/kernel.h>
#include <kernel/threading/unique_lock.h>
#include <stdarg.h>

influx::console *influx::console::get_console() { return _console; }

influx::console *influx::console::set_console(influx::console *console) {
    influx::console *old_console = _console;

    // Try to load the new console
    if (_console) {
        _log("Loading new console at address %p.\n", console);
    }
    if (!console->load()) {
        if (_console) {
            _log("Console load failed!\n");
        }
        return console;
    }

    // Set the new console
    _console = console;

    // Write the history to the new console
    if (_history.size() != 0) {
        console->stdout_write(_history);
        _log("Console history loaded into new console.\n");
    }

    // If the new console didn't ask to save history, delete it
    if (console->save_history()) {
        _history = "";
    }

    return old_console;
}

void influx::console::putchar(char c) { influx::console::putchar(output_stream::stdout, c); }

void influx::console::putchar(influx::output_stream stream, char c) {
    // Check for valid console
    kassert(_console != nullptr);

    threading::unique_lock lk(_mutex, threading::defer_lock);

    // If the scheduler is initialized, lock the mutex
    if (kernel::scheduler() != nullptr && kernel::scheduler()->started()) {
        lk.lock();
    }

    // Print by the stream type
    if (stream == output_stream::stdout) {
        // Save the char to the history
        if (_console->save_history()) {
            _history += c;
        }

        _console->stdout_putchar(c);
    } else if (stream == output_stream::stderr) {
        _console->stderr_putchar(c);
    }
}

void influx::console::vprint(const char *fmt, va_list args) {
    structures::string str;

    // Pass the va_args to the format function
    str = vformat(fmt, args);

    // Print the string to stdout
    print(output_stream::stdout, str);
}

void influx::console::print(influx::structures::string str) {
    influx::console::print(output_stream::stdout, str);
}

void influx::console::print(const char *fmt, ...) {
    structures::string str;

    va_list args;
    va_start(args, fmt);

    vprint(fmt, args);

    va_end(args);
}

void influx::console::print(influx::output_stream stream, influx::structures::string str) {
    // Check for valid console
    kassert(_console != nullptr);

    threading::unique_lock lk(_mutex, threading::defer_lock);

    // If the scheduler is initialized, lock the mutex
    if (kernel::scheduler() != nullptr && kernel::scheduler()->started()) {
        lk.lock();
    }

    // Print by the stream type
    if (stream == output_stream::stdout) {
        // Save the string to the history
        if (_console->save_history()) {
            _history += str;
        }

        _console->stdout_write(str);
    } else if (stream == output_stream::stderr) {
        _console->stderr_write(str);
    }
}

void influx::console::print(influx::output_stream stream, const char *fmt, ...) {
    structures::string str;

    va_list args;
    va_start(args, fmt);

    // Pass the va_args to the format function
    str = vformat(fmt, args);

    va_end(args);

    // Print the string
    print(stream, str);
}