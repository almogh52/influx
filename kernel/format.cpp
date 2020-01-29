#include <kernel/format.h>
#include <stdarg.h>

influx::structures::string influx::to_string(int64_t d, influx::integer_base base, bool sign,
                                             uint64_t padding) {
    structures::string str;

    uint64_t ud = (uint64_t)d;
    uint8_t remainder = 0;

    // Check if the integer is negetive and the base is decimal
    if (sign && base == integer_base::dec && d < 0) {
        // Set the integer as positive
        ud = (uint64_t)-d;
    }

    // Add padding if needed
    if (padding > 0) {
        str += structures::string(padding, '0');
    }

    do {
        // Get the remainder of the base
        remainder = (uint8_t)(ud % (uint8_t)base);

        // Add the matching character for the remainder
        str += (char)((remainder < 10) ? (char)remainder + '0' : (char)remainder + 'a' - 10);
    } while (ud /= (uint8_t)base);

    // Reverse the string
    str.reverse();

    // Add minus in the start if the number is negative
    if (sign && base == integer_base::dec && d < 0) {
        str = "-"_s + str;
    }

    // Check if the base is hex, if it is add 0x prefix
    if (base == integer_base::hex) {
        str = "0x"_s + str;
    }

    return str;
}

influx::integer_base influx::get_integer_base_for_character(char c) {
    switch (c) {
        case 'o':
            return integer_base::oct;

        case 'x':
            return integer_base::hex;

        default:
            return integer_base::dec;
    }
}

influx::structures::string influx::format(const char *fmt, ...) {
    structures::string str;

    va_list args;
    va_start(args, fmt);

    uint64_t specifier_len = 0;
    uint8_t padding = 0;
    bool pad_0 = false, long_int = false;
    char c = 0, e = 0;

    // While we didn't reach the end of the format
    while ((c = *fmt++) != 0) {
        // Reset flags, padding and specifier length
        padding = 0;
        specifier_len = 0;
        pad_0 = false, long_int = false;

        // If the character is a normal character
        if (c != '%') {
            str += c;
        } else {
            specifier_len++;

            // Get the next character
            e = *fmt++;
            specifier_len++;
            if (!e) {
                break;
            }

            // Check for 0 padding
            if (e == '0') {
                pad_0 = true;
                e = *fmt++;
                specifier_len++;
            }

            // Check for amount of padding
            if (pad_0 && e >= '0' && e <= '9') {
                padding = (uint8_t)(e - '0');
                e = *fmt++;
                specifier_len++;
            }

            // Check for long variable
            if (e == 'l') {
                long_int = true;
                e = *fmt++;
                specifier_len++;
            }

            // For each specifier
            switch (e) {
                case 'd':
                case 'i':
                case 'u':
                case 'o':
                case 'x':
                    str += to_string(long_int ? va_arg(args, int64_t) : va_arg(args, int32_t),
                                     get_integer_base_for_character(e), e != 'u', padding);
                    break;

                case 'c':
                    str += (char)va_arg(args, int);
                    break;

                case 's':
                    str += va_arg(args, const char *);
                    break;

				case 'S':
					str += *(structures::string *)va_arg(args, void *);
                    break;

                case 'p':
                    str += to_string(va_arg(args, int64_t), integer_base::hex, false);
                    break;

                case '%':
                    str += '%';
                    break;

                default:
                    // Add the string itself since unknown specifier
                    str += structures::string(fmt - specifier_len, specifier_len);
                    break;
            }
        }
    }

    va_end(args);

    return str;
}