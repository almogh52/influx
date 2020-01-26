#pragma once

#include <kernel/memory/memory.h>
#include <kernel/structures/bitmap.h>
#include <memory/paging.h>
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
    static void init(const boot_info &info);

    static int64_t alloc_page();

   private:
    inline static structures::bitmap _bitmap;

    static void parse_memory_map_to_bitmap(const boot_info_mem &mmap, structures::bitmap &bitmap);
};
};  // namespace memory
};  // namespace influx