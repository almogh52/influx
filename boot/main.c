#include "main.h"

#include "inc/multiboot2.h"
#include "screen.h"

void boot_main(unsigned long magic, unsigned long addr)
{
    // Clear the screen and initialize terminal variables
    init_tty();

    // Print Bootstrap info
    printf("%s Bootstrap, v%s\n", OS_NAME, BOOTSTRAP_VERSION);
    
    asm("hlt");
}