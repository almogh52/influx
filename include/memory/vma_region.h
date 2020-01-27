#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct vma_region {
	uint64_t base_addr;
	uint64_t size;
	uint8_t protection_flags;
	bool allocated;
} vma_region_t;