#pragma once

#include <sys/boot_info.h>

#define KERNEL_VIRTUAL_ADDRESS 0xffffff7f80000000

void *load_kernel(boot_info *info);
void add_kernel_memory_entry(boot_info_mem *mmap, uint64_t base_addr, uint64_t size);