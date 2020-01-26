#pragma once

#include <stdint.h>
#include <sys/boot_info.h>

namespace influx {
namespace memory {
class utils {
   public:
    static void *get_pml4();

    static uint64_t patch_page_address_set_value(uint64_t address);
    static uint64_t patch_page_address(uint64_t address);
    static uint64_t get_page_entry_index(uint64_t address);
    static uint64_t get_page_offset(uint64_t virtual_address);

    static uint64_t count_physical_memory(const boot_info_mem &mem_info);
    static uint64_t calc_amount_of_pages_for_bitmap(uint64_t bitmap_size);

    static void memset(void *ptr, uint8_t value, uint64_t amount);
    static void memcpy(void *dst, void *src, uint64_t amount);
};
};  // namespace memory
};  // namespace influx