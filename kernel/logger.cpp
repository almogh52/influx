#include <kernel/logger.h>

#include <kernel/console/console.h>
#include <kernel/format.h>

influx::logger::logger(influx::structures::string module_name) : _module_name(module_name) {}

void influx::logger::log(influx::structures::string str) const {
    console::print("[%S] %S\n", &_module_name, &str);
}

void influx::logger::log(const char *fmt, ...) const {
    structures::string str;

    va_list args;
    va_start(args, fmt);

    console::vprint(format("[%S] %%s\n", &_module_name, fmt), args);

    va_end(args);
}

void influx::logger::operator()(structures::string str) const { log(str); }

void influx::logger::operator()(const char *fmt, ...) const {
    structures::string str;

    va_list args;
    va_start(args, fmt);

    console::vprint(format("[%S] %s", &_module_name, fmt), args);

    va_end(args);
}