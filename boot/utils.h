#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct range {
	uint64_t start;
	uint64_t end;
} range;

bool strcmp(const char *s1, const char *s2);
void memcpy(const uint8_t *src, uint8_t *dst, uint64_t amount);
void memset(uint8_t *dst, uint8_t value, uint64_t amount);

uint64_t min(uint64_t a, uint64_t b);
uint64_t max(uint64_t a, uint64_t b);

range find_range_intersection(range a, range b);