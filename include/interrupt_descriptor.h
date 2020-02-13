#pragma once
#include <stdint.h>

#define INTERRUPT_GATE_TYPE 0b1110
#define TRAP_GATE_TYPE 0b1111

typedef struct interrupt_descriptor {
	uint16_t offset_1;
	uint16_t selector;
	union
	{
		uint16_t attributes;
		struct {
			uint16_t interrupt_stack_table_index : 3;
			uint16_t reserved : 5;
			uint16_t type : 4;
			uint16_t zero_1 : 1;
			uint16_t privilege_level : 2;
			uint16_t present : 1;
		};
	};
	uint16_t offset_2;
	uint32_t offset_3;
	uint32_t zero_2;
} interrupt_descriptor_t;
