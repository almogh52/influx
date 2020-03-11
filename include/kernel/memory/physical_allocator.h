#pragma once

#include <kernel/memory/memory.h>
#include <kernel/structures/bitmap.h>
#include <memory/paging.h>
#include <memory/vma_region.h>
#include <stdint.h>
#include <sys/boot_info.h>

#define EARLY_MEMORY_SIZE 0x3200000  // 50MiB
#define EARLY_BITMAP_SIZE (EARLY_MEMORY_SIZE / PAGE_SIZE)

#define EARLY_TEMP_BUFFER_SIZE (PAGE_SIZE * 4)
#define EARLY_TEMP_PAGE_BASE_ADDRESS (HIGHER_HALF_KERNEL_OFFSET + 0x1000)  // 1MiB offset

#define BITMAP_ADDRESS_OFFSET 0xA00000  // 10MiB

namespace influx {
namespace memory {
class physical_allocator {
   public:
    static void init(const boot_info_mem &mmap);

    static vma_region_t get_bitmap_region();

    static int64_t alloc_page(int64_t existing_page_index=-1);
    static int64_t alloc_consecutive_pages(uint64_t amount);

    static void free_page(uint64_t page_index);

   private:
    inline static uint8_t _bitmap_obj[sizeof(influx::structures::bitmap)] = {0};
    inline static structures::bitmap &_bitmap = reinterpret_cast<structures::bitmap&>(_bitmap_obj);

    static void parse_memory_map_to_bitmap(const boot_info_mem &mmap, structures::bitmap &bitmap);
};
};  // namespace memory
};  // namespace influx