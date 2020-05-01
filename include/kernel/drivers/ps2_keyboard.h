#pragma once
#include <kernel/drivers/driver.h>

#include <kernel/interrupts/interrupt_regs.h>
#include <kernel/key_code.h>
#include <kernel/key_event.h>

#define PS2_IRQ 0x1

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_REGISTER_PORT 0x64
#define PS2_COMMAND_REGISTER_PORT 0x64

#define PS2_STATUS_REGISTER_OUTPUT_BUFFER_MASK 0x1

#define PS2_KEY_RELEASE_MASK 0x80

namespace influx {
namespace drivers {
constexpr key_code key_codes[128] = {key_code::INVALID,
                                     key_code::ESC,
                                     key_code::NUM_1,
                                     key_code::NUM_2,
                                     key_code::NUM_3,
                                     key_code::NUM_4,
                                     key_code::NUM_5,
                                     key_code::NUM_6,
                                     key_code::NUM_7,
                                     key_code::NUM_8,
                                     key_code::NUM_9,
                                     key_code::NUM_0,
                                     key_code::DASH,
                                     key_code::EQUALS,
                                     key_code::BACKSPACE,
                                     key_code::TAB,
                                     key_code::Q,
                                     key_code::W,
                                     key_code::E,
                                     key_code::R,
                                     key_code::T,
                                     key_code::Z,
                                     key_code::U,
                                     key_code::I,
                                     key_code::O,
                                     key_code::P,
                                     key_code::LEFT_SQUARE_BRACKET,
                                     key_code::RIGHT_SQUARE_BRACKET,
                                     key_code::ENTER,
                                     key_code::CTRL,
                                     key_code::A,
                                     key_code::S,
                                     key_code::D,
                                     key_code::F,
                                     key_code::G,
                                     key_code::H,
                                     key_code::J,
                                     key_code::K,
                                     key_code::L,
                                     key_code::SEMICOLON,
                                     key_code::QUOTE,
                                     key_code::TICK,
                                     key_code::LEFT_SHIFT,
                                     key_code::BACKSLASH,
                                     key_code::Y,
                                     key_code::X,
                                     key_code::C,
                                     key_code::V,
                                     key_code::B,
                                     key_code::N,
                                     key_code::M,
                                     key_code::COMMA,
                                     key_code::DOT,
                                     key_code::SLASH,
                                     key_code::RIGHT_SHIFT,
                                     key_code::STAR,
                                     key_code::ALT,
                                     key_code::SPACE,
                                     key_code::CAPS_LOCK,
                                     key_code::F1,
                                     key_code::F2,
                                     key_code::F3,
                                     key_code::F4,
                                     key_code::F5,
                                     key_code::F6,
                                     key_code::F7,
                                     key_code::F8,
                                     key_code::F9,
                                     key_code::F10,
                                     key_code::NUM_LOCK,
                                     key_code::SCROLL_LOCK,
                                     key_code::KEY_7,
                                     key_code::KEY_8,
                                     key_code::KEY_9,
                                     key_code::KEY_MINUS,
                                     key_code::KEY_4,
                                     key_code::KEY_5,
                                     key_code::KEY_6,
                                     key_code::KEY_PLUS,
                                     key_code::KEY_1,
                                     key_code::KEY_2,
                                     key_code::KEY_3,
                                     key_code::KEY_0,
                                     key_code::KEY_DOT,
                                     key_code::INVALID,
                                     key_code::INVALID,
                                     key_code::INVALID,
                                     key_code::F11,
                                     key_code::F12,
                                     key_code::INVALID};

class ps2_keyboard : public driver {
   public:
    ps2_keyboard();

    virtual bool load();

    key_event wait_for_key_event();

   private:
    key_event key_to_key_event(uint8_t key);
};
};  // namespace drivers
};  // namespace influx