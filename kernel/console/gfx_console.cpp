#include <kernel/console/gfx_console.h>

#include <kernel/assert.h>
#include <kernel/console/early_console.h>
#include <kernel/console/gfx_u_vga16_font.h>
#include <kernel/drivers/graphics/bga/bga.h>
#include <kernel/kernel.h>
#include <kernel/memory/utils.h>
#include <kernel/memory/virtual_allocator.h>
#include <kernel/threading/unique_lock.h>
#include <memory/paging.h>
#include <memory/protection_flags.h>

// Ignore warnings
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wpacked"
#define SSFN_NOIMPLEMENTATION
#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include <ssfn.h>
#pragma GCC diagnostic pop

influx::gfx_console::gfx_console(const boot_info_framebuffer &multiboot_framebuffer)
    : _log("GFX Console"),
      _multiboot_framebuffer(multiboot_framebuffer),
      _framebuffer(nullptr),
      _framebuffer_height(),
      _framebuffer_width(0) {}

bool influx::gfx_console::load() {
    influx::drivers::graphics::bga *bga_driver =
        (drivers::graphics::bga *)kernel::driver_manager()->get_driver("BGA");

    uint32_t framebuffer_pitch = 0;

    // If there is a BGA driver, use it
    if (bga_driver) {
        // Enable the BGA
        bga_driver->enable(true);

        // Set framebuffer properties
        _framebuffer = bga_driver->video_memory();
        _framebuffer_height = BGA_SCREEN_HEIGHT;
        _framebuffer_width = BGA_SCREEN_WIDTH;
        _framebuffer_bpp = BGA_BPP;
        framebuffer_pitch = BGA_SCREEN_WIDTH * (BGA_BPP / 8);
    } else if (_multiboot_framebuffer.framebuffer_addr != EARLY_VIDEO_MEMORY_PHYSICAL_ADDRESS) {
        _framebuffer = memory::virtual_allocator::allocate(
            _multiboot_framebuffer.framebuffer_width * _multiboot_framebuffer.framebuffer_height *
                (_multiboot_framebuffer.framebuffer_bpp / 8),
            PROT_WRITE | PROT_READ, _multiboot_framebuffer.framebuffer_addr / PAGE_SIZE);
        _framebuffer_height = _multiboot_framebuffer.framebuffer_height;
        _framebuffer_width = _multiboot_framebuffer.framebuffer_width;
        _framebuffer_bpp = _multiboot_framebuffer.framebuffer_bpp;
        framebuffer_pitch = _multiboot_framebuffer.framebuffer_pitch;
    } else {
        return false;
    }

    // Set SSFN properties
    ssfn_font = (ssfn_font_t *)&u_vga16_sfn;  // Set font
    ssfn_dst_ptr = (uint8_t *)_framebuffer;   // Set video memory
    ssfn_dst_pitch = framebuffer_pitch;       // Set line size
    ssfn_dst_h = _framebuffer_height;         // Set screen height
    ssfn_dst_w = _framebuffer_width;          // Set screen width
    ssfn_fg = DEFAULT_FOREGROUND_COLOR;       // Set default color
    ssfn_x = 0;                               // Set start X coordinate
    ssfn_y = 0;                               // Set start Y coordinate

    return true;
}

void influx::gfx_console::putchar(char c) {
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
    if (ssfn_x >= GFX_AMOUNT_OF_COLUMNS * GLYPH_WIDTH) {
        new_line();  // Handle line overflow by breaking a line
    }
}

void influx::gfx_console::print(const influx::structures::string &str) {
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
            putchar(str[i]);
        }
    }
}

void influx::gfx_console::clear() {
    // Clear the video memory
    memory::utils::memset(_framebuffer, 0,
                          _framebuffer_height * _framebuffer_width * (_framebuffer_bpp / 8));
}

void influx::gfx_console::scroll() {
    // Move all lines from line 2, 1 up
    memory::utils::memcpy(ssfn_dst_ptr, ssfn_dst_ptr + ssfn_dst_pitch * GLYPH_HEIGHT,
                          (_framebuffer_height - GLYPH_HEIGHT) * ssfn_dst_pitch);

    // Clear the last row
    memory::utils::memset(ssfn_dst_ptr + ssfn_dst_pitch * (GFX_AMOUNT_OF_LINES - 1) * GLYPH_HEIGHT,
                          0, ssfn_dst_pitch * GLYPH_HEIGHT);
}

void influx::gfx_console::new_line() {
    // Reset X position
    ssfn_x = 0;

    // If reached the last line, just scroll
    if (ssfn_y >= _framebuffer_height - GLYPH_HEIGHT) {
        scroll();
    } else {
        ssfn_y += GLYPH_HEIGHT;
    }
}

void influx::gfx_console::set_foreground_color(influx::console_color color) const {
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