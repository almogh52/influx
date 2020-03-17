#include <kernel/console/bga_console.h>

#include <kernel/console/bga_u_vga16_font.h>
#include <kernel/kernel.h>
#include <kernel/memory/utils.h>
#include <kernel/threading/unique_lock.h>

// Ignore warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wpacked"
#define SSFN_NOIMPLEMENTATION
#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include <ssfn.h>
#pragma GCC diagnostic pop

influx::bga_console::bga_console()
    : _log("BGA Console"),
      _driver((drivers::graphics::bga *)kernel::driver_manager()->get_driver("BGA")) {}

bool influx::bga_console::load() {
    // Check that the drive is found
    if (!_driver) {
        _log("BGA driver not found!\n");
        return false;
    }

    // Enable the BGA
    _driver->enable(true);

    // Set SSFN properties
    ssfn_font = (ssfn_font_t *)&u_vga16_sfn;            // Set font
    ssfn_dst_ptr = (uint8_t *)_driver->video_memory();  // Set video memory
    ssfn_dst_pitch = SCREEN_WIDTH * sizeof(uint32_t);   // Set line size
    ssfn_dst_h = SCREEN_HEIGHT;                         // Set screen height
    ssfn_dst_w = SCREEN_WIDTH;                          // Set screen width
    ssfn_fg = DEFAULT_FOREGROUND_COLOR;                 // Set default color
    ssfn_x = 0;                                         // Set start X coordinate
    ssfn_y = 0;                                         // Set start Y coordinate

    return true;
}

void influx::bga_console::stdout_putchar(char c) {
    threading::unique_lock lk(_mutex, threading::defer_lock);

    // If the scheduler has started, lock the mutex
    if (kernel::scheduler() != nullptr && kernel::scheduler()->started()) {
        lk.lock();
    }

    // If the character is for a new line
    if (c == '\n' || c == '\r') {
        new_line();
        return;
    }

    // Print the character
    ssfn_putc(c);

    // Check for new line
    if (ssfn_x >= BGA_AMOUNT_OF_COLUMNS * GLYPH_WIDTH) {
        new_line();  // Handle line overflow by breaking a line
    }
}

void influx::bga_console::stdout_write(influx::structures::string &str) {
    console_color color;
    size_t str_len = str.length();

    // For each character, print it to the screen
    for (size_t i = 0; i < str_len; i++) {
        // If the character is the escape key and there are at least 2 more characters after it
        if (str[i] == 27 && str_len - i - 1 >= 2) {
            // Get the wanted color
            color = (console_color)(str[i + 2] - '0');

            // Set foregound color
            set_foreground_color(color);

            // Skip those characters
            i += 2;
        } else {
            stdout_putchar(str[i]);
        }
    }
}

void influx::bga_console::stdout_clear() {
    // Clear the video memory
    memory::utils::memset(_driver->video_memory(), 0,
                          SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
}

void influx::bga_console::scroll() {
    // Move all lines from line 2, 1 up
    memory::utils::memcpy(ssfn_dst_ptr, ssfn_dst_ptr + ssfn_dst_pitch * GLYPH_HEIGHT,
                          (SCREEN_HEIGHT - GLYPH_HEIGHT) * ssfn_dst_pitch);

    // Clear the last row
    memory::utils::memset(ssfn_dst_ptr + ssfn_dst_pitch * (BGA_AMOUNT_OF_LINES - 1) * GLYPH_HEIGHT,
                          0, ssfn_dst_pitch * GLYPH_HEIGHT);
}

void influx::bga_console::new_line() {
    // Reset X position
    ssfn_x = 0;

    // If reached the last line, just scroll
    if (ssfn_y >= SCREEN_HEIGHT - GLYPH_HEIGHT) {
        scroll();
    } else {
        ssfn_y += GLYPH_HEIGHT;
    }
}

void influx::bga_console::set_foreground_color(influx::console_color color) const {
    switch (color) {
        default:
        case console_color::clear:
            ssfn_fg = DEFAULT_FOREGROUND_COLOR;
            break;

        case console_color::gray:
            ssfn_fg = 0x616161;
            break;

        case console_color::blue:
            ssfn_fg = 0x2979ff;
            break;

        case console_color::green:
            ssfn_fg = 0x76ff03;
            break;

        case console_color::cyan:
            ssfn_fg = 0x00e5ff;
            break;

        case console_color::red:
            ssfn_fg = 0xff1744;
            break;

        case console_color::pink:
            ssfn_fg = 0xf50057;
            break;

        case console_color::yellow:
            ssfn_fg = 0xffea00;
            break;

        case console_color::white:
            ssfn_fg = DEFAULT_FOREGROUND_COLOR;
            break;
    }
}