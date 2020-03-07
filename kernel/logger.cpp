#include <kernel/logger.h>

#include <kernel/console/console.h>
#include <kernel/format.h>
#include <kernel/kernel.h>

influx::logger::logger(influx::structures::string module_name, console_color log_color)
    : _module_name(module_name), _log_color(log_color) {}

void influx::logger::vlog(const char *fmt, va_list args) const {
    structures::string time = format_time();

    console::vprint(format("[%S] [\033[%d%S\033[0] %s", &time, _log_color, &_module_name, fmt),
                    args);
}

void influx::logger::log(influx::structures::string str) const {
    structures::string time = format_time();

    console::print("[%S] [\033[%d%S\033[0] %S\n", &time, _log_color, &_module_name, &str);
}

void influx::logger::log(const char *fmt, ...) const {
    structures::string str;

    va_list args;
    va_start(args, fmt);

    // Log using the wanted args
    vlog(fmt, args);

    va_end(args);
}

void influx::logger::operator()(structures::string str) const { log(str); }

void influx::logger::operator()(const char *fmt, ...) const {
    structures::string str;

    va_list args;
    va_start(args, fmt);

    // Log using the wanted args
    vlog(fmt, args);

    va_end(args);
}

influx::structures::string influx::logger::format_time() const {
    // If the time manager wasn't loaded yet, return empty string
    if (kernel::time_manager() == nullptr) {
        return "00:00:00.0000";
    } else {
        return format("%02d:%02d:%02d.%04d",
                      (uint64_t)(kernel::time_manager()->seconds() / 60 * 60),
                      (uint64_t)(kernel::time_manager()->seconds() / 60) % 60,
                      (uint64_t)kernel::time_manager()->seconds() % 60,
                      (uint64_t)kernel::time_manager()->milliseconds());
    }
}