#pragma once

#include <memory/paging.h>
#include <stdint.h>

typedef struct buffer {
    void *ptr;
    uint64_t size;
} buffer_t;