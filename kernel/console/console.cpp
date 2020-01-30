#include <kernel/assert.h>
#include <kernel/console/console.h>
#include <kernel/format.h>
#include <stdarg.h>

influx::console *influx::console::set_console(influx::console *console) {
    influx::console *old_console = _console;

    // Set the new console
    _console = console;

    return old_console;
}

void influx::console::putchar(char c) { influx::console::putchar(output_stream::stdout, c); }

void influx::console::putchar(influx::output_stream stream, char c) {
    // Check for valid console
    kassert(_console != nullptr);

    // Print by the stream type
    if (stream == output_stream::stdout) {
        _console->stdout_putchar(c);
    } else if (stream == output_stream::stderr) {
        _console->stderr_putchar(c);
    }
}

void influx::console::print(influx::structures::string str) {
    influx::console::print(output_stream::stdout, str);
}

void influx::console::print(influx::output_stream stream, influx::structures::string str) {
    // Check for valid console
    kassert(_console != nullptr);

    // Print by the stream type
    if (stream == output_stream::stdout) {
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