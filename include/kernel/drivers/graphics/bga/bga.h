#pragma once
#include <kernel/drivers/driver.h>

#define BGA_PCI_DEVICE_ID 0x1111
#define BGA_PCI_VENDOR_ID 0x1234

#define BGA_SCREEN_HEIGHT 960
#define BGA_SCREEN_WIDTH 960
#define BGA_BPP 32

namespace influx {
namespace drivers {
namespace graphics {
class bga : public driver {
   public:
    bga();

    virtual bool load();

    void enable(bool clear_screen) const;
    void disable() const;

    void *video_memory() const { return _video_memory; }

   private:
    void *_video_memory;

    uint16_t read_register(uint16_t register_index) const;
    void write_register(uint16_t register_index, uint16_t value) const;
};
};  // namespace graphics
};  // namespace drivers
};  // namespace influx