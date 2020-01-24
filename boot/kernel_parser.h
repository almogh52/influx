#pragma once

#include <sys/boot_info.h>

#define KERNEL_VIRTUAL_ADDRESS 0xffffff7f80000000

void *load_kernel(boot_info *info);