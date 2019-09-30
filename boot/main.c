#include "main.h"

#include <multiboot2.h>
#include <stdint.h>

#include "screen.h"

void boot_main(uint32_t multiboot_magic, uint32_t multiboot_info_addr) {
    uint32_t *multiboot_info_ptr = (uint32_t *)(uint64_t)multiboot_info_addr;

    uint32_t multiboot_info_size;

    // Clear the screen and initialize terminal variables
    init_tty();

    // Print Bootstrap info
    printf("%s Bootstrap, v%s\n", OS_NAME, BOOTSTRAP_VERSION);

    // Check multiboot2 magic number
    printf("Multiboot2 magic number: 0x%x\n", multiboot_magic);
    if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        printf("Invalid Multiboot2 magic number!\n");
        return;
    }

    // Check if the MBI is aligned
    printf("Multiboot2 info structure address: 0x%x\n", multiboot_info_addr);
    if (multiboot_info_addr & 7) {
        printf("MBI not aligned!\n");
        return;
    }

    // Get the size of the multiboot information
    multiboot_info_size = *multiboot_info_ptr;
    printf("Multiboot2 info structure size: 0x%x\n", multiboot_info_size);

    // Skip to the first tag
    multiboot_info_ptr += 2;

    // While we didn't reach the end of the multiboot information structure
    printf("Parsing Multiboot2 information structure..\n");
    for (struct multiboot_tag *tag = (struct multiboot_tag *)multiboot_info_ptr;
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag + ((int)(tag->size + 7) & ~7))) {
        printf("Multiboot2 tag: type = %d, size = 0x%x\n", tag->type, tag->size);

        // If the tag is the boot loader name
        if (tag->type == MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME) {
            printf("Boot loader: %s\n", ((struct multiboot_tag_string *)tag)->string);
        } else if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)  // If the tag is the memory map
        {
            // For each memory map entry, print it's details
            for (struct multiboot_mmap_entry *mmap = ((struct multiboot_tag_mmap *)tag)->entries;
                 (multiboot_uint8_t *)mmap < (multiboot_uint8_t *)tag + tag->size;
                 mmap =
                     (multiboot_memory_map_t *)((unsigned long)mmap +
                                                ((struct multiboot_tag_mmap *)tag)->entry_size)) {
                printf("Memory map entry: base_addr = 0x%x%x, length = 0x%x%x, type = 0x%x\n",
                       (unsigned)(mmap->addr >> 32), (unsigned)(mmap->addr & 0xffffffff),
                       (unsigned)(mmap->len >> 32), (unsigned)(mmap->len & 0xffffffff),
                       (unsigned)mmap->type);
            }
        } else if (tag->type == MULTIBOOT_TAG_TYPE_MODULE)  // If the tag is a module tag
        {
            struct multiboot_tag_module *module_tag = (struct multiboot_tag_module *)tag;

            printf("Multiboot2 module: mod_start = 0x%x, mod_end = 0x%x, name = %s\n",
                   module_tag->mod_start, module_tag->mod_end, module_tag->cmdline);
        }
    }

    asm("hlt");
}