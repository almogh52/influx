#pragma once
#include <kernel/console/console.h>

#include <kernel/console/color.h>
#include <kernel/memory/memory.h>
#include <kernel/threading/mutex.h>
#include <memory/vma_region.h>
#include <stdint.h>

#define EARLY_VIDEO_MEMORY_PHYSICAL_ADDRESS 0xB8000
#define EARLY_VIDEO_MEMORY_ADDRESS (HIGHER_HALF_KERNEL_OFFSET + EARLY_VIDEO_MEMORY_PHYSICAL_ADDRESS)

#define AMOUNT_OF_COLUMNS 80
#define AMOUNT_OF_LINES 25
#define DEFAULT_ATTRIBUTE 7

namespace influx {
class early_console : public console {
   public:
    early_console();

    virtual bool load();

    virtual void print(const structures::string &str);
    virtual void clear();

    static vma_region get_vma_region();

   private:
    unsigned char *_video;
    threading::mutex _mutex;

    uint8_t _attribute;
    uint8_t _x_pos;
    uint8_t _y_pos;

    void scroll();
    void new_line();

    void putchar(char c);

    uint8_t attribute_to_color(console_color color) const;
};
};  // namespace influx