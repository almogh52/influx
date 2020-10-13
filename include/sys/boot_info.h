#pragma once

#include <stdint.h>

#define MAX_MEM_ENTRIES 30

typedef struct boot_info_kernel_module {
    uint64_t start_addr;
    uint64_t size;
} boot_info_kernel_module;

typedef enum boot_info_mem_entry_type { 
    AVAILABLE,
    RESERVED,
    KERNEL
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

typedef struct boot_info_framebuffer {
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_height;
    uint32_t framebuffer_width;
    uint8_t framebuffer_bpp;
} boot_info_framebuffer;

typedef struct boot_info {
    boot_info_kernel_module kernel_module;
    boot_info_mem memory;
    boot_info_framebuffer framebuffer;
    uint64_t tss_address;
    char *cmdline;
} boot_info;