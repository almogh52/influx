#pragma once
#include <kernel/logger.h>
#include <kernel/structures/string.h>
#include <kernel/threading/mutex.h>
#include <stdarg.h>

namespace influx {
enum class output_stream { stdout, stderr };

class console {
   public:
    static console *get_console();
    static console *set_console(console *console);

    static void putchar(char c);
    static void putchar(output_stream stream, char c);

    static void vprint(const char *fmt, va_list args);
    static void print(structures::string str);
    static void print(const char *fmt, ...);
    static void print(output_stream stream, structures::string str);
    static void print(output_stream stream, const char *fmt, ...);

    virtual bool load() = 0;

    virtual void stdout_putchar(char c) = 0;
    virtual void stdout_write(structures::string &str) = 0;
    virtual void stdout_clear() = 0;

    virtual void stderr_putchar(char c) = 0;
    virtual void stderr_write(structures::string &str) = 0;
    virtual void stderr_clear() = 0;

    virtual bool save_history() const = 0;

    // virtual char stdin_getchar() = 0;

   private:
    inline static logger _log = logger("Console Manager", console_color::green);
    inline static threading::mutex _mutex;
    inline static console *_console = nullptr;
    inline static structures::string _history = "";
};
};  // namespace influx