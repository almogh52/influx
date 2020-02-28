#pragma once
#include <stdint.h>

typedef struct pci_descriptor {
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t class_code;
	uint8_t subclass;
	uint8_t prog_if;
} pci_descriptor_t;