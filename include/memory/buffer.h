#pragma once

#include <stdint.h>

typedef struct buffer {
    void *ptr;
    uint64_t size;
} buffer_t;