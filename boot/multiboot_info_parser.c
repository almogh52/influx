#include "multiboot_info_parser.h"

#include <multiboot2.h>

#include "screen.h"
#include "utils.h"

boot_info parse_multiboot_info(uint32_t *multiboot_info_ptr) {
    boot_info info = {0};
    
    uint32_t multiboot_info_size;

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
                info.memory.entries[info.memory.entry_count].base_addr = mmap->addr;
                info.memory.entries[info.memory.entry_count].size = mmap->len;
                info.memory.entries[info.memory.entry_count].type =
                    mmap->type == MULTIBOOT_MEMORY_AVAILABLE ? AVAILABLE : RESERVED;
                info.memory.entry_count++;

                printf("Memory map entry: base_addr = 0x%x%x, length = 0x%x%x, type = 0x%x\n",
                       (unsigned)(mmap->addr >> 32), (unsigned)(mmap->addr & 0xffffffff),
                       (unsigned)(mmap->len >> 32), (unsigned)(mmap->len & 0xffffffff),
                       (unsigned)mmap->type);
            }
        } else if (tag->type == MULTIBOOT_TAG_TYPE_MODULE)  // If the tag is a module tag
        {
            struct multiboot_tag_module *module_tag = (struct multiboot_tag_module *)tag;

            // Check if we found the kernel module
            if (strcmp(module_tag->cmdline, KERNEL_BIN_STR)) {
                info.kernel_module.start_addr = module_tag->mod_start;
                info.kernel_module.size = module_tag->mod_end - module_tag->mod_start;
            }

            printf("Multiboot2 module: mod_start = 0x%x, mod_end = 0x%x, name = %s\n",
                   module_tag->mod_start, module_tag->mod_end, module_tag->cmdline);
        } else if (tag->type == MULTIBOOT_TAG_TYPE_CMDLINE)  // If the tag is a cmdline tag
        {
            struct multiboot_tag_string *cmdline_tag = (struct multiboot_tag_string *)tag;

            // Save the cmdline
            info.cmdline = cmdline_tag->string;

            printf("Multiboot2 cmdline: %s\n", cmdline_tag->string);
        }
    }

    return info;
}