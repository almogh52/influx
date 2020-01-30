#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <memory/protection_flags.h>

typedef struct vma_region {
	uint64_t base_addr;
	uint64_t size;
	protection_flags_t protection_flags;
	bool allocated;
} vma_region_t;