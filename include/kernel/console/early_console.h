#pragma once
#include <kernel/console/console.h>

#include <kernel/console/color.h>
#include <kernel/memory/memory.h>
#include <memory/vma_region.h>
#include <stdint.h>

#define EARLY_VIDEO_MEMORY_ADDRESS (HIGHER_HALF_KERNEL_OFFSET + 0xB8000)

#define AMOUNT_OF_COLUMNS 80
#define AMOUNT_OF_LINES 25
#define DEFAULT_ATTRIBUTE 7

namespace influx {
class early_console : public console {
   public:
    early_console();

    virtual void stdout_putchar(char c);
    virtual void stdout_write(structures::string &str);
    virtual void stdout_clear();

    inline virtual void stderr_putchar(char c) { stdout_putchar(c); }
    inline virtual void stderr_write(structures::string &str) { stdout_write(str); }
    inline virtual void stderr_clear() { stderr_clear(); }

    static vma_region get_vma_region();

   private:
    unsigned char *_video;

    uint8_t _attribute;
    uint8_t _x_pos;
    uint8_t _y_pos;

    void scroll();
    void new_line();

    uint8_t attribute_to_color(console_color color) const;
};
};  // namespace influx