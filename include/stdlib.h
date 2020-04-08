#ifndef _INFLUX_STDLIB_H_
#define _INFLUX_STDLIB_H_
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t new_size);
void free(void* ptr);

int memcmp(const void* ptr1, const void* ptr2, size_t num);
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* destination, const void* source, size_t num);

#ifdef __cplusplus
}
#endif

#endif