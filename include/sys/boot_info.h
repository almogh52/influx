#pragma once

#include <stdint.h>

#define MAX_MEM_ENTRIES 30

typedef struct boot_info_kernel_module {
    uint64_t start_addr;
    uint64_t size;
} boot_info_kernel_module;

typedef enum boot_info_mem_entry_type { 
    AVAILABLE,
    RESERVED
} boot_info_mem_entry_type;

typedef struct boot_info_mem_entry {
    uint64_t base_addr;
    uint64_t size;
    boot_info_mem_entry_type type;
} boot_info_mem_entry;

typedef struct boot_info_mem {
    uint32_t entry_count;
    boot_info_mem_entry entries[MAX_MEM_ENTRIES];
} boot_info_mem;

typedef struct boot_info {
    boot_info_kernel_module kernel_module;
    boot_info_mem memory;
    char *cmdline;
} boot_info;