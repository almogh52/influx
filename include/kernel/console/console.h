#pragma once

#include <kernel/structures/string.h>

namespace influx {
enum class output_stream {
	stdout,
	stderr
};

class console {
   public:
	static console *set_console(console *console);

	static void putchar(char c);
	static void putchar(output_stream stream, char c);
	static void print(structures::string str);
	static void print(output_stream stream, structures::string str);
	static void print(output_stream stream, const char *fmt, ...);

    virtual void stdout_putchar(char c) = 0;
    virtual void stdout_write(structures::string &str) = 0;
	virtual void stdout_clear() = 0;

    virtual void stderr_putchar(char c) = 0;
    virtual void stderr_write(structures::string &str) = 0;
	virtual void stderr_clear() = 0;

    //virtual char stdin_getchar() = 0;

	private:
	inline static console *_console = nullptr;
};
};  // namespace influx