#pragma once

#include <stdint.h>
#include <stdbool.h>

bool strcmp(const char *s1, const char *s2);
void memcpy(const uint8_t *src, uint8_t *dst, uint64_t amount);
void memset(uint8_t *dst, uint8_t value, uint64_t amount);