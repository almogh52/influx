#pragma once

#include <memory/buffer.h>
#include <memory/paging.h>

#define PML4T_ADDRESS 0xfffffffffffff000
#define PDP_TABLES_BASE 0xffffffffffe00000
#define PD_TABLES_BASE 0xffffffffc0000000
#define PT_TABLES_BASE 0xffffff8000000000

#define PAGE_MAPPING_BUFFER_SIZE (PAGE_SIZE * 8)

namespace influx {
namespace memory {
class paging_manager {
   public:
    static pml4e_t *get_pml4e(uint64_t address);
    static pdpe_t *get_pdpe(uint64_t address);
    static pde_t *get_pde(uint64_t address);
    static pte_t *get_pte(uint64_t address);

    static uint64_t get_physical_address(uint64_t virtual_address);

    //static void map_page(uint64_t page_base_address, uint64_t page_index);
    static bool map_page(uint64_t page_base_address, uint64_t page_index, buffer_t &buf,
                         uint64_t buf_physical_address);

    static bool temp_map_page(uint64_t page_base_address, buffer_t &buf,
                              uint64_t buf_physical_address, int64_t page_index = -1);
    static void unmap_temp_mapping(uint64_t page_base_address, uint64_t buf_physical_address,
                                   uint64_t buf_size);

   private:
    /*inline static char _mapping_buffer[PAGE_MAPPING_BUFFER_SIZE];
    inline static uint64_t _mapping_buffer_offset = 0;*/

    static void invalidate_page(uint64_t page_base_virtual_address);
};
};  // namespace memory
};  // namespace influx