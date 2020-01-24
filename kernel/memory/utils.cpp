#include <kernel/memory/utils.h>

#include <memory/paging.h>

void *influx::memory::utils::get_pml4() {
    uint64_t cr3;

    // Read control register 3 to get pml4 address
    __asm__ __volatile__("mov %0, cr3;" : "=r"(cr3) : :);

    return (void *)cr3;
}

uint64_t influx::memory::utils::patch_page_address_set_value(uint64_t address)
{
    return address >> 12;
}

uint64_t influx::memory::utils::patch_page_address(uint64_t address) {
    return address & 0xFFFFFFFFFF000;
}

uint64_t influx::memory::utils::get_page_entry_index(uint64_t address) { return address & 0x1FF; }

uint64_t influx::memory::utils::count_physical_memory(boot_info_mem &mem_info) {
    uint64_t mem_size = 0;

    // For each entry add it's size to the mem size
    for (int i = 0; i < (int)mem_info.entry_count; i++) {
        mem_size += mem_info.entries[i].size;
    }

    return mem_size;
}

uint64_t influx::memory::utils::calc_amount_of_pages_for_bitmap(uint64_t bitmap_size) {
    return (bitmap_size / PAGE_SIZE) + (bitmap_size % PAGE_SIZE ? 1 : 0);
}