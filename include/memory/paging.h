#pragma once

#include <stdint.h>

#define PAGE_SIZE 4096
#define AMOUNT_OF_PAGE_TABLE_ENTRIES 512

#define READ_ONLY_ACCESS 0
#define READ_WRITE_ACCESS 1

#define SUPERVISOR_ONLY_ACCESS 0
#define SUPERVISOR_USER_ACCESS 1

#define WRITEBACK_CACHING_POLICY 0
#define WRITETHROUGH_CACHING_POLICY 1

typedef struct pml4e {
    union {
        uint64_t raw;
        struct {
            uint64_t present : 1;
            uint64_t read_write : 1;
            uint64_t user_supervisor : 1;
            uint64_t page_writethrough : 1;
            uint64_t page_cache_disable : 1;
            uint64_t accessed : 1;
            uint64_t ignore : 1;
            uint64_t zero : 2;
            uint64_t available1 : 3;
            uint64_t address_placeholder : 40;
            uint64_t available2 : 11;
            uint64_t no_execute : 1;
        };
        struct {
            uint64_t pdp_address : 52;
        };
    };
} pml4e_t;

typedef struct pdpe {
    union {
        uint64_t raw;
        struct {
            uint64_t present : 1;
            uint64_t read_write : 1;
            uint64_t user_supervisor : 1;
            uint64_t page_writethrough : 1;
            uint64_t page_cache_disable : 1;
            uint64_t accessed : 1;
            uint64_t ignore : 1;
			uint64_t page_size : 1;
            uint64_t zero : 1;
            uint64_t available1 : 3;
            uint64_t address_placeholder : 40;
            uint64_t available2 : 11;
            uint64_t no_execute : 1;
        };
        struct {
            uint64_t pd_address : 52;
        };
    };
} pdpe_t;

typedef struct pde {
    union {
        uint64_t raw;
        struct {
            uint64_t present : 1;
            uint64_t read_write : 1;
            uint64_t user_supervisor : 1;
            uint64_t page_writethrough : 1;
            uint64_t page_cache_disable : 1;
            uint64_t accessed : 1;
            uint64_t ignore1 : 1;
            uint64_t page_size : 1;
            uint64_t ignore2 : 1;
            uint64_t available1 : 3;
            uint64_t address_placeholder : 40;
            uint64_t available2 : 11;
            uint64_t no_execute : 1;
        };
        struct {
            uint64_t pt_address : 52;
        };
    };
} pde_t;

typedef struct pte {
    union {
        uint64_t raw;
        struct {
            uint64_t present : 1;
            uint64_t read_write : 1;
            uint64_t user_supervisor : 1;
            uint64_t page_writethrough : 1;
            uint64_t page_cache_disable : 1;
            uint64_t accessed : 1;
            uint64_t dirty : 1;
            uint64_t page_attribute_table : 1;
            uint64_t global : 1;
            uint64_t available1 : 3;
            uint64_t address_placeholder : 40;
            uint64_t available2 : 11;
            uint64_t no_execute : 1;
        };
        struct {
            uint64_t page_address : 52;
        };
    };
} pte_t;