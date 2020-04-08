#include <stdlib.h>

#include <kernel/memory/liballoc.h>
#include <kernel/memory/utils.h>

void *malloc(size_t size) { return kmalloc(size); }
void *calloc(size_t num, size_t size) { return kcalloc(num, size); }
void *realloc(void *ptr, size_t new_size) { return krealloc(ptr, new_size); }
void free(void *ptr) { return kfree(ptr); }

int memcmp(const void *ptr1, const void *ptr2, size_t num) {
    return influx::memory::utils::memcmp(ptr1, ptr2, num);
}

void *memset(void *ptr, int value, size_t num) {
    influx::memory::utils::memset(ptr, (uint8_t)value, num);
    return ptr;
}

void *memcpy(void *destination, const void *source, size_t num) {
    influx::memory::utils::memcpy(destination, source, num);
    return destination;
}