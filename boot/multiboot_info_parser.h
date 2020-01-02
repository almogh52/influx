#pragma once

#include <stdint.h>
#include <sys/boot_info.h>

#define KERNEL_BIN_STR "KERNEL_BIN"

boot_info parse_multiboot_info(uint32_t *multiboot_info_ptr);