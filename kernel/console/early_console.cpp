#include <kernel/console/early_console.h>

influx::early_console::early_console()
    : _video((unsigned char *)EARLY_VIDEO_MEMORY_ADDRESS), _x_pos(0), _y_pos(0) {
    // Disable cursor using hardware ports
    __asm__ __volatile__(
        "mov dx, 0x3D4;"
        "mov al, 0xA;"  // Low cursor shape register
        "out dx, al;"
        "inc dx;"
        "mov al, 0x20;"  // Bits 6-7 unused, bit 5 disables the cursor, bits 0-4 control the cursor
                         // shape
        "out dx, al"
        :
        :
        : "dx", "al");

    // Clear the video memory
    stdout_clear();
}

void influx::early_console::stdout_putchar(char c) {
    // If the character is for a new line
    if (c == '\n' || c == '\r') {
        new_line();
        return;
    }

    // Set the character with default attribute
    *(_video + (_x_pos + _y_pos * AMOUNT_OF_COLUMNS) * 2) = (unsigned char)c & 0xFF;
    *(_video + (_x_pos + _y_pos * AMOUNT_OF_COLUMNS) * 2 + 1) = DEFAULT_ATTRIBUTE;

    // Increase the X position and check for new line
    _x_pos++;
    if (_x_pos >= AMOUNT_OF_COLUMNS) {
        new_line();  // Handle line overflow by breaking a line
    }
}

void influx::early_console::stdout_write(influx::structures::string &str)
{
	size_t str_len = str.length();

	// For each character, print it to the screen
	for (size_t i = 0; i < str_len; i++) {
		stdout_putchar(str[i]);
	}
}

void influx::early_console::stdout_clear() {
    // For each character and attribute set 0
    for (uint16_t i = 0; i < (AMOUNT_OF_COLUMNS * AMOUNT_OF_LINES * 2) / sizeof(uint64_t); i++) {
        *((uint64_t *)_video + i) = 0;
    }
}

void influx::early_console::scroll() {
    // Move all lines from line 2, 1 up
    for (uint16_t i = (AMOUNT_OF_COLUMNS * 2) / sizeof(uint64_t);
         i < (AMOUNT_OF_COLUMNS * AMOUNT_OF_LINES * 2) / sizeof(uint64_t); i++) {
        *((uint64_t *)_video + i - (AMOUNT_OF_COLUMNS * 2) / sizeof(uint64_t)) =
            *((uint64_t *)_video + i);
    }

    // Clear the last row
    for (uint16_t i = (AMOUNT_OF_COLUMNS * (AMOUNT_OF_LINES - 1) * 2) / sizeof(uint64_t);
         i < (AMOUNT_OF_COLUMNS * (AMOUNT_OF_LINES + 1) * 2) / sizeof(uint64_t); i++) {
        *((uint64_t *)_video + i) = 0;
    }
}

void influx::early_console::new_line() {
    // Reset X position
    _x_pos = 0;

    // If reached the last line, just scroll
    if (_y_pos == AMOUNT_OF_LINES - 1) {
        scroll();
    } else {
        _y_pos++;
    }
}

vma_region influx::early_console::get_vma_region()
{
    return {.base_addr = EARLY_VIDEO_MEMORY_ADDRESS,
            .size = AMOUNT_OF_COLUMNS * AMOUNT_OF_LINES * 2,
            .protection_flags = PROT_READ | PROT_WRITE,
            .allocated = true};
}