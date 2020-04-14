#include <kernel/memory/physical_allocator.h>

#include <kernel/memory/paging_manager.h>
#include <kernel/memory/utils.h>
#include <kernel/structures/static_bitmap.h>
#include <memory/buffer.h>
#include <memory/protection_flags.h>

void influx::memory::physical_allocator::init(const boot_info_mem &mmap) {
    structures::static_bitmap<EARLY_BITMAP_SIZE> early_bitmap;

    char temp_buffer_raw[EARLY_TEMP_BUFFER_SIZE] __attribute__((aligned(0x1000)));
    buffer_t temp_buffer{.ptr = (void *)temp_buffer_raw, .size = EARLY_TEMP_BUFFER_SIZE};
    uint64_t temp_buffer_physical_address =
        paging_manager::get_physical_address((uint64_t)temp_buffer_raw);

    buffer_t structures_buffer;
    uint64_t structures_page_index = 0;

    uint64_t bitmap_size = (utils::count_physical_memory(mmap) / PAGE_SIZE) + 1;
    uint64_t amount_of_memory_pages = utils::calc_amount_of_pages_for_bitmap(bitmap_size);
    uint64_t page_chunk_start = 0;

    // Parse the memory map to the early bitmap
    parse_memory_map_to_bitmap(mmap, early_bitmap);

    // Search a batch of pages to allocate the bitmap in
    if (!early_bitmap.search(amount_of_memory_pages, 0, page_chunk_start)) {
        __asm__ __volatile__("hlt");
    }

    // Allocate the pages for the bitmap
    early_bitmap.set(page_chunk_start, amount_of_memory_pages, true);

    // Search for initial structures page
    if (!early_bitmap.search_bit(0, structures_page_index)) {
        __asm__ __volatile__("hlt");
    }

    // Allocate the structures page
    early_bitmap.set(structures_page_index, true);

    // Temporarily map the structures page
    if (!paging_manager::temp_map_page(EARLY_TEMP_PAGE_BASE_ADDRESS, temp_buffer,
                                       temp_buffer_physical_address, structures_page_index)) {
        __asm__ __volatile__("hlt");
    }

    // Set R/W access to the page
    paging_manager::set_pte_permissions(EARLY_TEMP_PAGE_BASE_ADDRESS, PROT_READ | PROT_WRITE);

    // Create the buffer for the strucutures page
    structures_buffer = {.ptr = (void *)EARLY_TEMP_PAGE_BASE_ADDRESS, .size = PAGE_SIZE};

    // Map the pages for the bitmap
    for (uint64_t i = 0; i < amount_of_memory_pages; i++) {
        if (!paging_manager::map_page(
                HIGHER_HALF_KERNEL_OFFSET + BITMAP_ADDRESS_OFFSET + i * PAGE_SIZE,
                page_chunk_start + i, structures_buffer, structures_page_index * PAGE_SIZE)) {
            // Unmap the structures page
            paging_manager::unmap_temp_mapping(
                EARLY_TEMP_PAGE_BASE_ADDRESS, temp_buffer_physical_address, EARLY_TEMP_BUFFER_SIZE, false);

            // Search for a page for the structures
            if (!early_bitmap.search_bit(0, structures_page_index)) {
                __asm__ __volatile__("hlt");
            }

            // Allocate the structures page
            early_bitmap.set(structures_page_index, true);

            // Temporarily map the structures page
            if (!paging_manager::temp_map_page(EARLY_TEMP_PAGE_BASE_ADDRESS, temp_buffer,
                                               temp_buffer_physical_address,
                                               structures_page_index)) {
                __asm__ __volatile__("hlt");
            }

            // Create the buffer for the strucutures page
            structures_buffer = {.ptr = (void *)EARLY_TEMP_PAGE_BASE_ADDRESS, .size = PAGE_SIZE};

            paging_manager::get_physical_address(
                (uint64_t)paging_manager::get_pte(0xffffff7f80C40000));

            // Try to map the page again, if failed halt
            if (!paging_manager::map_page(
                    HIGHER_HALF_KERNEL_OFFSET + BITMAP_ADDRESS_OFFSET + i * PAGE_SIZE,
                    page_chunk_start + i, structures_buffer, structures_page_index * PAGE_SIZE)) {
                __asm__ __volatile__("hlt");
            }
        }

        // Set R/W access to the page
        paging_manager::set_pte_permissions(
            HIGHER_HALF_KERNEL_OFFSET + BITMAP_ADDRESS_OFFSET + i * PAGE_SIZE,
            PROT_READ | PROT_WRITE);
    }

    // Unmap the structures page
    paging_manager::unmap_temp_mapping(EARLY_TEMP_PAGE_BASE_ADDRESS,
                                       structures_page_index * PAGE_SIZE, PAGE_SIZE, false);

    // Init bitmap object
    _bitmap = structures::bitmap((void *)(HIGHER_HALF_KERNEL_OFFSET + BITMAP_ADDRESS_OFFSET),
                                 bitmap_size);

    // Parse the memory map to the bitmap
    parse_memory_map_to_bitmap(mmap, _bitmap);

    // Copy the early bitmap
    utils::memcpy(_bitmap.get_raw(), early_bitmap.get_raw(), EARLY_BITMAP_SIZE);
}

vma_region_t influx::memory::physical_allocator::get_bitmap_region() {
    uint64_t bytes_for_bitmap =
        utils::calc_amount_of_pages_for_bitmap(_bitmap.size()) * PAGE_SIZE / 8;

    return {.base_addr = (HIGHER_HALF_KERNEL_OFFSET + BITMAP_ADDRESS_OFFSET),
            .size = bytes_for_bitmap,
            .protection_flags = PROT_READ | PROT_WRITE,
            .allocated = true};
}

int64_t influx::memory::physical_allocator::alloc_page(int64_t existing_page_index) {
    uint64_t page_index = existing_page_index >= 0 ? existing_page_index : 0;

    bool found = existing_page_index < 0 ? _bitmap.search_bit(0, page_index) : true;

    // If found, allocate the page
    if (found) {
        _bitmap[page_index] = true;
    }

    return found ? page_index : -1;
}

int64_t influx::memory::physical_allocator::alloc_consecutive_pages(uint64_t amount) {
    uint64_t first_page_index = 0;

    bool found = _bitmap.search(amount, 0, first_page_index);

    // If found, allocate the page
    if (found) {
        _bitmap.set(first_page_index, amount, 1);
    }

    return found ? first_page_index : -1;
}

void influx::memory::physical_allocator::free_page(uint64_t page_index) {
    if (page_index < _bitmap.size()) {
        _bitmap[page_index] = false;
    }
}

void influx::memory::physical_allocator::parse_memory_map_to_bitmap(const boot_info_mem &mmap,
                                                                    structures::bitmap<> &bitmap) {
    // Set all pages as used
    bitmap.set(0, bitmap.size(), true);

    // For each memory entry, set it's pages status in the bitmap
    for (uint32_t i = 0; i < mmap.entry_count; i++) {
        // If the memory entry is reserved or the kernel itself, mark it's pages as taken
        if (mmap.entries[i].type == RESERVED || mmap.entries[i].type == KERNEL) {
            bitmap.set(mmap.entries[i].base_addr / PAGE_SIZE,
                       mmap.entries[i].size % PAGE_SIZE ? (mmap.entries[i].size / PAGE_SIZE) + 1
                                                        : mmap.entries[i].size / PAGE_SIZE,
                       true);
        } else {
            bitmap.set(
                mmap.entries[i].base_addr % PAGE_SIZE ? (mmap.entries[i].base_addr / PAGE_SIZE) + 1
                                                      : mmap.entries[i].base_addr / PAGE_SIZE,
                mmap.entries[i].base_addr % PAGE_SIZE
                    ? mmap.entries[i].size / PAGE_SIZE ? (mmap.entries[i].size / PAGE_SIZE) - 1 : 0
                    : mmap.entries[i].size / PAGE_SIZE,
                false);
        }
    }
}