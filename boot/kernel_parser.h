#pragma once

#include <sys/boot_info.h>

#define KERNEL_VIRTUAL_ADDRESS 0xffffffff80000000

void *load_kernel(boot_info *info);