#pragma once

#include <stdint.h>

typedef struct descriptor_table_register {
	uint16_t limit;
	uint64_t base_virtual_address;
} __attribute__((packed)) descriptor_table_register_t;