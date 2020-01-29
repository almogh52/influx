#include <kernel/memory/utils.h>
#include <memory/paging.h>

void *influx::memory::utils::get_pml4() {
    uint64_t cr3;

    // Read control register 3 to get pml4 address
    __asm__ __volatile__("mov %0, cr3;" : "=r"(cr3) : :);

    return (void *)cr3;
}

uint64_t influx::memory::utils::patch_page_address_set_value(uint64_t address) {
    return address >> 12;
}

uint64_t influx::memory::utils::patch_page_address(uint64_t address) {
    return address & 0xFFFFFFFFFF000;
}

uint64_t influx::memory::utils::get_page_entry_index(uint64_t address) { return address & 0x1FF; }

uint64_t influx::memory::utils::get_page_offset(uint64_t virtual_address) {
    return virtual_address & 0xFFF;
}

uint64_t influx::memory::utils::count_physical_memory(const boot_info_mem &mem_info) {
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

void influx::memory::utils::memset(void *ptr, uint8_t value, uint64_t amount) {
    for (uint64_t i = 0; i < amount; i++) {
        *((uint8_t *)ptr + i) = value;
    }
}

void influx::memory::utils::memcpy(void *dst, const void *src, uint64_t amount) {
    // Copy chunks of 8 byte from src to dst
    for (uint64_t i = 0; i < amount / sizeof(uint64_t); i++) {
        *((uint64_t *)dst + i) = *((uint64_t *)src + i);
    }

    // Check if there is more bytes remaining to copy
    if (amount % sizeof(uint64_t)) {
        for (uint8_t i = 0; i < amount % sizeof(uint64_t); i++) {
            *((uint8_t *)dst + amount - ((amount % sizeof(uint64_t)) - i)) =
                *((uint8_t *)src + amount - ((amount % sizeof(uint64_t)) - i));
        }
    }
}