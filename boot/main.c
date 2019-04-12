#include "main.h"

#include "inc/multiboot2.h"
#include "screen.h"

void boot_main(unsigned long magic, unsigned long addr)
{
    // Clear the screen and initialize terminal variables
    init_tty();

    // Print Bootstrap info
    printf("%s Bootstrap, v%s\n", OS_NAME, BOOTSTRAP_VERSION);

    // Check multiboot2 magic number
    printf("Multiboot2 Magic Number: 0x%x\n", magic);
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
    {
        printf("Invalid Multiboot2 magic number!\n");
        return;
    }

    // Check if the MBI is aligned
    printf("Multiboot2 Info Structure Address: 0x%x\n", addr);
    if (addr & 7)
    {
        printf("MBI not aligned!\n");
        return;
    }
    
    asm("hlt");
}