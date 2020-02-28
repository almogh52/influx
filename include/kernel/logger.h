#pragma once

#include <kernel/structures/string.h>
#include <stdarg.h>

namespace influx {
class logger {
   public:
    logger(structures::string module_name);

    void log(structures::string str) const;
    void log(const char *fmt, ...) const;
    void operator()(structures::string str) const;
    void operator()(const char *fmt, ...) const;

   private:
    structures::string _module_name;
};
};  // namespace influx