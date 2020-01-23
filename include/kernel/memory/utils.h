#pragma once

#include <stdint.h>
#include <sys/boot_info.h>

namespace influx {
namespace memory {
class utils {
   public:
    static void *get_pml4();

    static uint64_t patch_page_entry_address(uint64_t address);

    static uint64_t count_physical_memory(boot_info_mem &mem_info);
    static uint64_t calc_amount_of_pages_for_bitmap(uint64_t bitmap_size);
};
};  // namespace memory
};  // namespace influx