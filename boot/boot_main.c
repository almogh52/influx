#include "inc/multiboot2.h"

void boot_main(unsigned long magic, unsigned long addr)
{
    asm("hlt");
}