#include "utils.h"

bool strcmp(const char *s1, const char *s2) {
    unsigned int i = 0;

    // While we didn't reach the end of one of the strings
    while (*(s1 + i) && *(s2 + i)) {
        // If the current char in each string differ
        if (*(s1 + i) != *(s2 + i)) {
            return false;
        }

        i++;
    }

    return *s1 == *s2;
}

void memcpy(const uint8_t *src, uint8_t *dst, uint64_t amount) {
    // Copy all bytes from src to dst
    for (uint64_t i = 0; i < amount; i++) {
        *(dst + i) = *(src + i);
    }
}

void memset(uint8_t *dst, uint8_t value, uint64_t amount) {
    // Set all bytes to the wanted value
    for (uint64_t i = 0; i < amount; i++) {
        *(dst + i) = value;
    }
}

uint64_t min(uint64_t a, uint64_t b) { return a > b ? b : a; }

uint64_t max(uint64_t a, uint64_t b) { return a > b ? a : b; }

range find_range_intersection(range a, range b) {
    range intersection = {.start = max(a.start, b.start), .end = min(a.end, b.end)};

    return intersection;
}