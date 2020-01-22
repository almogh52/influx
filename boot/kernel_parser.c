#include "kernel_parser.h"

#include <elf.h>
#include <stdbool.h>

#include "screen.h"
#include "utils.h"

bool verify_kernel_elf(Elf64_Ehdr *header);
void load_segments(void *kernel_file, Elf64_Phdr *pheader, unsigned int num_of_pheaders,
                   boot_info_mem *mmap);
void add_kernel_memory_entry(boot_info_mem *mmap, uint64_t base_addr, uint64_t size);

void *load_kernel(boot_info *info) {
    void *kernel_file = (void *)info->kernel_module.start_addr;
    Elf64_Ehdr *header = (Elf64_Ehdr *)kernel_file;

    // Verify the kernel's elf
    if (!verify_kernel_elf(header)) {
        return 0;
    }

    printf("Loading Kernel's Segments..\n");
    load_segments(kernel_file, (Elf64_Phdr *)((char *)kernel_file + header->e_phoff),
                  header->e_phnum, &info->memory);

    return (void *)header->e_entry;
}

bool verify_kernel_elf(Elf64_Ehdr *header) {
    // Verify ELF Magic Number
    printf("Verifying Kernel's ELF Magic Number..\n");
    if (header->e_magic0 != ELFMAG0 || header->e_magic1 != ELFMAG1 || header->e_magic2 != ELFMAG2 ||
        header->e_magic3 != ELFMAG3) {
        printf("Kernel's ELF Magic Number is invalid\n");
        return false;
    }

    // Verify ELF class
    printf("Verifying Kernel's ELF is 64-bit..\n");
    if (header->e_class != ELFCLASS64) {
        printf("Kernel's ELF isn't 64-bit\n");
        return false;
    }

    // Verify ELF arch
    printf("Verifying Kernel's ELF Arch..\n");
    if (header->e_machine != 0x3E) {
        printf("Kernel's ELF Arch isn't x86_64\n");
        return false;
    }

    return true;
}

void load_segments(void *kernel_file, Elf64_Phdr *pheader, unsigned int num_of_pheaders,
                   boot_info_mem *mmap) {
    Elf64_Phdr *header;

    for (unsigned int i = 0; i < num_of_pheaders; i++) {
        header = pheader + i;

        printf(
            "ELF Program Header: type = 0x%x, offset = 0x%lx, vaddr = 0x%lx, filesz = 0x%lx, memsz "
            "= "
            "0x%lx\n",
            header->p_type, header->p_offset, header->p_vaddr, header->p_filesz, header->p_memsz);

        // If the program header type is to load data
        if (header->p_type == PT_LOAD) {
            // Copy the data to the wanted location
            memcpy((uint8_t *)((char *)kernel_file + header->p_offset), (uint8_t *)header->p_vaddr,
                   header->p_filesz);

            // Set all 0s in the remaining data
            memset((uint8_t *)(header->p_vaddr + header->p_offset + header->p_filesz), 0,
                   header->p_memsz - header->p_filesz);

            // Add kernel memory entry
            add_kernel_memory_entry(mmap, header->p_vaddr - KERNEL_VIRTUAL_ADDRESS, header->p_memsz);
        }
    }
}

void add_kernel_memory_entry(boot_info_mem *mmap, uint64_t base_addr, uint64_t size) {
    boot_info_mem new_mmap = {0};

    boot_info_mem_entry *entry = 0;

    range kernel_entry_range = {.start = base_addr, .end = base_addr + size};
    range entry_range;
    range entry_intersection = {0};

    // For each original entry, check if it conflicts with the kernel entry
    for (uint16_t i = 0; i < mmap->entry_count; i++) {
        entry = (boot_info_mem_entry *)mmap->entries + i;
        entry_range = (range){.start = entry->base_addr, .end = entry->base_addr + entry->size};

        // Check if there is an intersection between the kernel entry and the entry
        entry_intersection = find_range_intersection(entry_range, kernel_entry_range);

        // If there isn't an entry intersection, just add the entry
        if (entry_intersection.end <= entry_intersection.start) {
            new_mmap.entries[new_mmap.entry_count++] = *entry;
        } else {
            // If the intersection starts where the entry starts
            if (entry_intersection.start == entry_range.start) {
                // If the intersection is the entire entry, just add the kernel entry instead
                if (entry_intersection.end == (entry->base_addr + entry->size)) {
                    new_mmap.entries[new_mmap.entry_count++] = (boot_info_mem_entry){
                        .base_addr = entry->base_addr, .size = entry->size, .type = KERNEL};
                } else {
                    // Add the kernel entry
                    new_mmap.entries[new_mmap.entry_count++] = (boot_info_mem_entry){
                        .base_addr = entry->base_addr,
                        .size = entry_intersection.end - entry_intersection.start,
                        .type = KERNEL};

                    // Add the rest of the entry
                    new_mmap.entries[new_mmap.entry_count++] = (boot_info_mem_entry){
                        .base_addr = entry_intersection.end,
                        .size = entry->size - (entry_intersection.end - entry_intersection.start),
                        .type = entry->type};
                }
            }
            // If the intersection starts in the middle of the entry
            else if (entry_intersection.start > entry_range.start) {
                // If the intersection ends in the end of the entry
                if (entry_intersection.end == (entry->base_addr + entry->size)) {
                    // Add the rest of the entry
                    new_mmap.entries[new_mmap.entry_count++] = (boot_info_mem_entry){
                        .base_addr = entry->base_addr,
                        .size = entry->size - (entry_intersection.end - entry_intersection.start),
                        .type = entry->type};

                    // Add the kernel entry
                    new_mmap.entries[new_mmap.entry_count++] = (boot_info_mem_entry){
                        .base_addr = entry_intersection.start,
                        .size = entry_intersection.end - entry_intersection.start,
                        .type = KERNEL};
                } else {
                    // Add the start of the entry
                    new_mmap.entries[new_mmap.entry_count++] = (boot_info_mem_entry){
                        .base_addr = entry->base_addr,
                        .size = entry_intersection.start - entry->base_addr,
                        .type = entry->type};

                    // Add the kernel entry
                    new_mmap.entries[new_mmap.entry_count++] = (boot_info_mem_entry){
                        .base_addr = entry_intersection.start,
                        .size = entry_intersection.end - entry_intersection.start,
                        .type = KERNEL};

                    // Add the end of the entry
                    new_mmap.entries[new_mmap.entry_count++] = (boot_info_mem_entry){
                        .base_addr = entry_intersection.end,
                        .size = entry->base_addr + entry->size - entry_intersection.end,
                        .type = entry->type};
                }
            }
        }
    }

    // Set the new memory map
    *mmap = new_mmap;
}