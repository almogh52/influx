#pragma once
#include <kernel/console/console.h>

#include <kernel/console/color.h>
#include <kernel/drivers/graphics/bga/bga.h>
#include <kernel/logger.h>
#include <stdint.h>

#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 16

#define BGA_AMOUNT_OF_COLUMNS (SCREEN_WIDTH / GLYPH_WIDTH)
#define BGA_AMOUNT_OF_LINES (SCREEN_HEIGHT / GLYPH_HEIGHT)

#define DEFAULT_FOREGROUND_COLOR 0xFFFFFF

namespace influx {
class bga_console : public console {
   public:
    bga_console();

    virtual bool load();

    virtual void stdout_putchar(char c);
    virtual void stdout_write(structures::string &str);
    virtual void stdout_clear();

    inline virtual void stderr_putchar(char c) { stdout_putchar(c); }
    inline virtual void stderr_write(structures::string &str) { stdout_write(str); }
    inline virtual void stderr_clear() { stderr_clear(); }

   private:
    logger _log;
    drivers::graphics::bga *_driver;

    void scroll();
    void new_line();

    void set_foreground_color(console_color color) const;
};
};  // namespace influx