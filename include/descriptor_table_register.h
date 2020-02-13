#pragma once

#include <stdint.h>

typedef struct descriptor_table_register {
	uint64_t base_virtual_address;
	uint16_t limit;
} descriptor_table_register_t;