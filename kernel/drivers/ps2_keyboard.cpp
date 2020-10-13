#include <kernel/drivers/ps2_keyboard.h>

#include <kernel/kernel.h>
#include <kernel/ports.h>

influx::drivers::ps2_keyboard::ps2_keyboard() : driver("PS/2 Keyboard") {}

bool influx::drivers::ps2_keyboard::load() {
    // Clean input buffer
    while ((ports::in<uint8_t>(PS2_STATUS_REGISTER_PORT) &
            PS2_STATUS_REGISTER_OUTPUT_BUFFER_MASK) == PS2_STATUS_REGISTER_OUTPUT_BUFFER_MASK) {
        ports::in<uint8_t>(PS2_DATA_PORT);
    }

    return true;
}

influx::key_event influx::drivers::ps2_keyboard::wait_for_key_event() {
    // Wait for IRQ
    while (!kernel::interrupt_manager()->wait_for_irq(PS2_IRQ, false))
        ;

    return key_to_key_event(ports::in<uint8_t>(PS2_DATA_PORT));
}

influx::key_event influx::drivers::ps2_keyboard::key_to_key_event(uint8_t key) {
    return key_event{.raw_key = key,
                     .code = key_codes[key & ~PS2_KEY_RELEASE_MASK],
                     .released = (bool)(key & PS2_KEY_RELEASE_MASK)};
}