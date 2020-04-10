#pragma once
#include <stdint.h>

typedef struct tss {
    uint32_t reserved_0;
    uint32_t rsp0_low;
    uint32_t rsp0_high;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved_1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved_2;
    uint16_t reserved_3;
    uint16_t io_map_base_address;
} __attribute__((packed)) tss_t;