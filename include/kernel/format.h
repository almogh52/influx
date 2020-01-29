#pragma once

#include <kernel/structures/string.h>
#include <stdbool.h>

namespace influx {
enum class integer_base { oct = 8, dec = 10, hex = 16 };

structures::string to_string(int64_t n, integer_base base = integer_base::dec, bool sign = true,
                             uint64_t padding = 0);

integer_base get_integer_base_for_character(char c);

structures::string format(const char *fmt, ...);
constexpr auto fmt = format;
};  // namespace influx