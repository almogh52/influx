#pragma once

#include <kernel/memory/memory.h>
#include <kernel/structures/node.h>
#include <memory/buffer.h>
#include <memory/vma_region.h>
#include <sys/boot_info.h>

#define KERNEL_VMA_START HIGHER_HALF_KERNEL_OFFSET
#define KERNEL_VMA_SIZE 0x8000000000 // 512GiB - The size of PML4E

#define VMA_LIST_INITIAL_ADDRESS (HIGHER_HALF_KERNEL_OFFSET + 0x900000)  // 9MiB offset

inline bool operator==(const vma_region_t &a, const vma_region_t &b) { return a.base_addr == b.base_addr; }
inline bool operator<(const vma_region_t &a, const vma_region_t &b) { return a.base_addr < b.base_addr; }
inline bool operator>(const vma_region_t &a, const vma_region_t &b) { return a.base_addr > b.base_addr; }

namespace influx {
namespace memory {
typedef structures::node<vma_region_t> vma_node_t;

class virtual_allocator {
   public:
    static void init(const boot_info_mem &mmap);

    static void *allocate(uint64_t size, protection_flags_t pflags, int64_t physical_page_index=-1);
    static void free(void *ptr, uint64_t size);

   private:
    inline static vma_node_t *_vma_list_head = nullptr;
    inline static buffer_t _current_vma_list_page = {.ptr = nullptr, .size = 0};

    static vma_node_t *alloc_vma_node(vma_region_t region);

    static void insert_vma_region(vma_region_t region);
    static bool free_vma_region(vma_region_t region);

    static void check_for_vma_node_combination(vma_node_t *node);

    static bool address_in_vma_region(vma_region_t &region, uint64_t &address);
    static vma_region_t find_free_region(uint64_t size, protection_flags_t pflags);

    static void *allocate(vma_region_t region, int64_t physical_page_index=-1);
};
};  // namespace memory
};  // namespace influx