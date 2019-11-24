#pragma once

#include <stdint.h>

#include "../include/boot_info.h"

boot_info *parse_multiboot_info(uint32_t *multiboot_info_ptr);