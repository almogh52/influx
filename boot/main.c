#include "main.h"

#include <multiboot2.h>
#include <stdint.h>

#include "multiboot_info_parser.h"
#include "screen.h"

void boot_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    uint32_t *multiboot_info_ptr = (uint32_t *)(uint64_t)multiboot_info_addr;

    boot_info *info = 0;

    // Clear the screen and initialize terminal variables
    init_tty();

    // Print Bootstrap info
    printf("%s Bootstrap, v%s\n\n", OS_NAME, BOOTSTRAP_VERSION);

    // Check multiboot2 magic number
    printf("Multiboot2 magic number: 0x%x\n", multiboot_magic);
    if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        printf("Invalid Multiboot2 magic number!\n");
        return;
    }

    // Check if the MBI is aligned
    printf("Multiboot2 info structure address: 0x%x\n", multiboot_info_addr);
    if (multiboot_info_addr & 7) {
        printf("Multiboot2 info structure not aligned!\n");
        return;
    }

    // Parse the multiboot structure
    info = parse_multiboot_info(multiboot_info_ptr);
    if (info) {
    }

    asm("hlt");
}