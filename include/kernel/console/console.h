#pragma once
#include <kernel/logger.h>
#include <kernel/structures/string.h>
#include <kernel/threading/mutex.h>
#include <stdarg.h>

namespace influx {
class console {
   public:
    static console *get_console();
    static console *set_console(console *console);

    virtual bool load() = 0;

    virtual void print(const structures::string &str) = 0;
    virtual void clear() = 0;

    virtual uint64_t text_columns() const = 0;
    virtual uint64_t text_rows() const = 0;

   private:
    inline static logger _log = logger("Console Manager", console_color::green);
    inline static threading::mutex _console_mutex;
    inline static console *_console = nullptr;
};
};  // namespace influx