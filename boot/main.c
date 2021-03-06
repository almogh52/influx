#include "main.h"

#include <multiboot2.h>
#include <stdint.h>

#include "kernel_parser.h"
#include "multiboot_info_parser.h"
#include "screen.h"

extern uint64_t main_paging[], main_paging_end[], gdt64[], gdt64_end[], tss64[], tss64_end[],
    stack_bottom[], stack_top[];

void boot_main(uint32_t multiboot_magic, uint32_t multiboot_info_old_addr) {
    uint64_t multiboot_info_addr = multiboot_info_old_addr + HIGHER_HALF_OFFSET;
    uint32_t *multiboot_info_ptr = (uint32_t *)(uint64_t)multiboot_info_addr;

    boot_info info;

    void *kernel_entry_ptr = 0;

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

    // Load the kernel into the memory
    kernel_entry_ptr = load_kernel(&info);

    // Add the paging structures, the GDT, the TSS and the stack as part of the kernel memory
    add_kernel_memory_entry(&info.memory, (uint64_t)((uint64_t)main_paging - HIGHER_HALF_OFFSET),
                            (uint64_t)((uint64_t)main_paging_end - (uint64_t)main_paging));
    add_kernel_memory_entry(&info.memory, (uint64_t)((uint64_t)gdt64 - HIGHER_HALF_OFFSET),
                            (uint64_t)((uint64_t)gdt64_end - (uint64_t)gdt64));
    add_kernel_memory_entry(&info.memory, (uint64_t)((uint64_t)tss64 - HIGHER_HALF_OFFSET),
                            (uint64_t)((uint64_t)tss64_end - (uint64_t)tss64));
    add_kernel_memory_entry(&info.memory, (uint64_t)((uint64_t)stack_bottom - HIGHER_HALF_OFFSET),
                            (uint64_t)((uint64_t)stack_top - (uint64_t)stack_bottom));

    // Set the TSS address
    info.tss_address = (uint64_t)tss64;

    // Call the kernel entry point
    printf("Calling kernel's entry point..\n");
    ((boot_info(*)())kernel_entry_ptr)(info);

    // Re-init tty if returend from kernel
    init_tty();

    // Halt
    printf("Returned from kernel! Halting..\n");
    asm("cli; hlt");
}