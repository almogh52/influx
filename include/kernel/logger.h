#pragma once

#include <kernel/console/color.h>
#include <kernel/structures/string.h>
#include <stdarg.h>

namespace influx {
class logger {
   public:
    logger(structures::string module_name, console_color log_color = console_color::cyan);

    void log(structures::string str) const;
    void log(const char *fmt, ...) const;
    void operator()(structures::string str) const;
    void operator()(const char *fmt, ...) const;

   private:
    structures::string _module_name;
    console_color _log_color;
};
};  // namespace influx