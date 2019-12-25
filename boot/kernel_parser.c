#include "kernel_parser.h"

#include <stdbool.h>
#include <elf.h>

#include "screen.h"
#include "utils.h"

bool verify_kernel_elf(Elf64_Ehdr *header);
void load_segments(void *kernel_file, Elf64_Phdr *pheader, unsigned int num_of_pheaders);

void *load_kernel(boot_info info) {
    void *kernel_file = (void *)info.kernel_module.start_addr;
    Elf64_Ehdr *header = (Elf64_Ehdr *)kernel_file;

    // Verify the kernel's elf
    if (!verify_kernel_elf(header)) {
        return 0;
    }

    printf("Loading Kernel's Segments..\n");
    load_segments(kernel_file, (Elf64_Phdr *)((char *)kernel_file + header->e_phoff), header->e_phnum);

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

void load_segments(void *kernel_file, Elf64_Phdr *pheader, unsigned int num_of_pheaders) {
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
        }
    }
}