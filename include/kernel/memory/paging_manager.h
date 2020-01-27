#pragma once

#include <kernel/memory/memory.h>
#include <memory/buffer.h>
#include <memory/paging.h>
#include <memory/protection_flags.h>
#include <memory/vma_region.h>

#define PML4T_ADDRESS 0xfffffffffffff000
#define PDP_TABLES_BASE 0xffffffffffe00000
#define PD_TABLES_BASE 0xffffffffc0000000
#define PT_TABLES_BASE 0xffffff8000000000

#define STRUCTURES_BUFFER_ADDRESS (HIGHER_HALF_KERNEL_OFFSET + 0x1000)
#define STRUCTURES_BUFFER_SIZE (PAGE_SIZE * 16)

namespace influx {
namespace memory {
class paging_manager {
   public:
    static vma_region_t get_bitmap_region();

    static pml4e_t *get_pml4e(uint64_t address);
    static pdpe_t *get_pdpe(uint64_t address);
    static pde_t *get_pde(uint64_t address);
    static pte_t *get_pte(uint64_t address);

    static uint64_t get_physical_address(uint64_t virtual_address);

    static bool map_page(uint64_t page_base_address);
    static bool map_page(uint64_t page_base_address, uint64_t page_index, buffer_t &buf,
                         uint64_t buf_physical_address);

    static bool temp_map_page(uint64_t page_base_address, buffer_t &buf,
                              uint64_t buf_physical_address, int64_t page_index = -1);
    static void unmap_temp_mapping(uint64_t page_base_address, uint64_t buf_physical_address,
                                   uint64_t buf_size);

    static void set_pte_permissions(uint64_t virtual_address, protection_flags_t pflags);

   private:
    inline static char _structures_mapping_temp_buffer[PAGE_SIZE] __attribute__((aligned(0x1000)));
    inline static buffer_t _structures_buffer = {.ptr = nullptr, .size = 0};

    static bool allocate_structures_buffer();
    static void free_structures_buffer();

    static void invalidate_page(uint64_t page_base_virtual_address);
};
};  // namespace memory
};  // namespace influx