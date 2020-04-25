#pragma once
#include <kernel/console/console.h>

#include <kernel/console/color.h>
#include <kernel/logger.h>
#include <kernel/threading/mutex.h>
#include <stdint.h>
#include <sys/boot_info.h>

#define GLYPH_WIDTH 8
#define GLYPH_HEIGHT 16

#define GFX_AMOUNT_OF_COLUMNS (_framebuffer_width / GLYPH_WIDTH)
#define GFX_AMOUNT_OF_LINES (_framebuffer_height / GLYPH_HEIGHT)

#define DEFAULT_FOREGROUND_COLOR 0xFFFFFF

namespace influx {
class gfx_console : public console {
   public:
    gfx_console(const boot_info_framebuffer &multiboot_framebuffer);

    virtual bool load();

    virtual void print(const structures::string &str);
    virtual void clear();

    inline virtual uint64_t text_columns() const { return _framebuffer_width / GLYPH_WIDTH; }
    inline virtual uint64_t text_rows() const { return _framebuffer_height / GLYPH_HEIGHT; }

   private:
    logger _log;

    boot_info_framebuffer _multiboot_framebuffer;

    void *_framebuffer;
    uint32_t _framebuffer_height;
    uint32_t _framebuffer_width;
    uint8_t _framebuffer_bpp;

    threading::mutex _mutex;

    void scroll();
    void new_line();

    void putchar(char c);

    void set_foreground_color(console_color color) const;
};
};  // namespace influx