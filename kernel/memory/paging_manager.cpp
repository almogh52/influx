
#include <kernel/memory/paging_manager.h>

#include <kernel/memory/physical_allocator.h>
#include <kernel/memory/utils.h>
#include <stdint.h>

vma_region_t influx::memory::paging_manager::get_bitmap_region() {
    return {.base_addr = STRUCTURES_BUFFER_ADDRESS,
            .size = STRUCTURES_BUFFER_SIZE,
            .protection_flags = PROT_READ | PROT_WRITE,
            .allocated = true};
}

pml4e_t *influx::memory::paging_manager::get_pml4e(uint64_t address) {
    return (pml4e_t *)PML4T_ADDRESS + utils::get_page_entry_index(address >> 39);
}

pdpe_t *influx::memory::paging_manager::get_pdpe(uint64_t address) {
    return (pdpe_t *)(PDP_TABLES_BASE + 0x1000 * utils::get_page_entry_index(address >> 39)) +
           utils::get_page_entry_index(address >> 30);
}

pde_t *influx::memory::paging_manager::get_pde(uint64_t address) {
    return (pde_t *)(PD_TABLES_BASE + 0x200000 * utils::get_page_entry_index(address >> 39) +
                     0x1000 * utils::get_page_entry_index(address >> 30)) +
           utils::get_page_entry_index(address >> 21);
}

pte_t *influx::memory::paging_manager::get_pte(uint64_t address) {
    return (pte_t *)(PT_TABLES_BASE + 0x40000000 * utils::get_page_entry_index(address >> 39) +
                     0x200000 * utils::get_page_entry_index(address >> 30) +
                     0x1000 * utils::get_page_entry_index(address >> 21)) +
           utils::get_page_entry_index(address >> 12);
}

uint64_t influx::memory::paging_manager::get_physical_address(uint64_t virtual_address) {
    pml4e_t *pml4e = get_pml4e(virtual_address);
    pdpe_t *pdpe = get_pdpe(virtual_address);
    pde_t *pde = get_pde(virtual_address);
    pte *pte = get_pte(virtual_address);

    return pml4e->present && pdpe->present && pde->present && pte->present
               ? utils::patch_page_address(pte->page_address) +
                     utils::get_page_offset(virtual_address)
               : 0;
}

bool influx::memory::paging_manager::map_page(uint64_t page_base_address, int64_t page_index) {
    bool mapped = false;

    uint64_t structures_buffer_physical_address =
        get_physical_address((uint64_t)_structures_buffer.ptr);

    // If no physical page was given, allocate one
    page_index = physical_allocator::alloc_page(page_index);
    if (page_index < 0) {
        return false;
    }

    // If the buffer for the structures is empty, allocate a new one
    if (_structures_buffer.size == 0 ||
        !(mapped = map_page(page_base_address, page_index, _structures_buffer,
                            structures_buffer_physical_address))) {
        // Free the current one
        free_structures_buffer();

        // Allocate new one
        allocate_structures_buffer();

        // Get the physical address of the structures buffer
        structures_buffer_physical_address = get_physical_address((uint64_t)_structures_buffer.ptr);
    }

    return mapped ? mapped
                  : map_page(page_base_address, page_index, _structures_buffer,
                             structures_buffer_physical_address);
}

bool influx::memory::paging_manager::map_page(uint64_t page_base_address, uint64_t page_index,
                                              buffer_t &buf, uint64_t buf_physical_address) {
    uint64_t buf_physical_offset = 0;

    pml4e_t *pml4e = nullptr;
    pdpe_t *pdpe = nullptr;
    pde_t *pde = nullptr;
    pte_t *pte = nullptr;

    // Get the PML4E for the temp page
    pml4e = get_pml4e(page_base_address);
    if (!pml4e->present) {
        // Check if the buffer has sufficient size for the PDPT
        if (buf.size >= AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pdpe_t)) {
            // Allocate PDPT
            utils::memset(buf.ptr, 0, AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pdpe_t));

            // Increase the page buffer offset and decrease it's size
            buf.ptr = (uint8_t *)buf.ptr + AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pdpe_t);
            buf.size -= AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pdpe_t);
        } else {
            // Unmap the temp mapping
            unmap_temp_mapping(page_base_address, buf_physical_address,
                               buf.size + buf_physical_offset);

            return false;
        }

        // Create the PML4E
        pml4e->address_placeholder = utils::patch_page_address_set_value(
                                         (uint64_t)(buf_physical_address + buf_physical_offset)) &
                                     0xFFFFFFFFFF;
        pml4e->read_write = READ_WRITE_ACCESS;
        pml4e->present = true;
        pml4e->no_execute = false;

        // Increase the buf physical offset
        buf_physical_offset += AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pdpe_t);
    }

    // Get the PDPE
    pdpe = get_pdpe(page_base_address);
    if (!pdpe->present) {
        // Check if the buffer has sufficient size for the PDT
        if (buf.size >= AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pde_t)) {
            // Allocate PDT
            utils::memset(buf.ptr, 0, AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pde_t));

            // Increase the page buffer offset and decrease it's size
            buf.ptr = (uint8_t *)buf.ptr + AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pde_t);
            buf.size -= AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pde_t);
        } else {
            // Unmap the temp mapping
            unmap_temp_mapping(page_base_address, buf_physical_address,
                               buf.size + buf_physical_offset);

            return false;
        }

        // Create the PDPE and allocate the PDT
        pdpe->address_placeholder = utils::patch_page_address_set_value(
                                        (uint64_t)(buf_physical_address + buf_physical_offset)) &
                                    0xFFFFFFFFFF;
        pdpe->read_write = READ_WRITE_ACCESS;
        pdpe->present = true;
        pdpe->no_execute = false;

        // Increase the buf physical offset
        buf_physical_offset += AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pde_t);
    }

    // Get the PDE
    pde = get_pde(page_base_address);
    if (!pde->present) {
        // Check if the buffer has sufficient size for the PT
        if (buf.size >= AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pte_t)) {
            // Allocate PT
            utils::memset(buf.ptr, 0, AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pte_t));

            // Increase the page buffer offset and decrease it's size
            buf.ptr = (uint8_t *)buf.ptr + AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pte_t);
            buf.size -= AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pte_t);
        } else {
            // Unmap the temp mapping
            unmap_temp_mapping(page_base_address, buf_physical_address,
                               buf.size + buf_physical_offset);

            return false;
        }

        // Create the PDE and allocate the PT
        pde->address_placeholder = utils::patch_page_address_set_value(
                                       (uint64_t)(buf_physical_address + buf_physical_offset)) &
                                   0xFFFFFFFFFF;
        pde->read_write = READ_WRITE_ACCESS;
        pde->present = true;
        pde->no_execute = false;

        // Increase the buf physical offset
        buf_physical_offset += AMOUNT_OF_PAGE_TABLE_ENTRIES * sizeof(pde_t);
    }

    // Get the PTE
    pte = get_pte(page_base_address);

    // Create the PTE and point it to the page
    pte->address_placeholder =
        utils::patch_page_address_set_value(page_index * PAGE_SIZE) & 0xFFFFFFFFFF;
    pte->read_write = READ_ONLY_ACCESS;
    pte->present = true;
    pte->no_execute = true;

    return true;
}

void influx::memory::paging_manager::unmap_page(uint64_t page_base_address) {
    pml4e_t *pml4e = get_pml4e(page_base_address);
    pdpe_t *pdpe = get_pdpe(page_base_address);
    pde_t *pde = get_pde(page_base_address);
    pte *pte = get_pte(page_base_address);

    // If the PT exists
    if (pml4e->present && pdpe->present && pde->present) {
        *(pte) = (pte_t){0};

        // Invalidate the TLB for the page
        invalidate_page(page_base_address);
    }
}

bool influx::memory::paging_manager::temp_map_page(uint64_t page_base_address, buffer_t &buf,
                                                   uint64_t buf_physical_address,
                                                   int64_t page_index) {
    // If no page index was selected for the temp page, allocate a page for it
    if (page_index < 0 && (page_index = physical_allocator::alloc_page()) < 0) {
        return false;
    }

    // Map the page to it's virtual address
    return map_page(page_base_address, page_index, buf, buf_physical_address);
}

void influx::memory::paging_manager::unmap_temp_mapping(uint64_t page_base_address,
                                                        uint64_t buf_physical_address,
                                                        uint64_t buf_size) {
    pml4e_t *pml4e = get_pml4e(page_base_address);

    pdpe_t *pdpe = nullptr;
    pde_t *pde = nullptr;
    pte_t *pte = nullptr;

    uint64_t data_structure_physical_address = 0;

    // If the PML4E isn't present
    if (!pml4e->present) {
        return;
    }

    // Get the PDPT physical address and check if it's in the buffer
    data_structure_physical_address = utils::patch_page_address(pml4e->pdp_address);
    if (data_structure_physical_address >= buf_physical_address &&
        data_structure_physical_address < (buf_physical_address - buf_size)) {
        // If the PDPT is in the buffer, disable the PML4E
        *pml4e = (pml4e_t){0};
    }

    // Get the PDPE
    pdpe = get_pdpe(page_base_address);
    if (!pdpe->present) {
        return;
    }

    // Get the PDT physical address and check if it's in the buffer
    data_structure_physical_address = utils::patch_page_address(pdpe->pd_address);
    if (data_structure_physical_address >= buf_physical_address &&
        data_structure_physical_address < (buf_physical_address - buf_size)) {
        // If the PDT is in the buffer, disable the PDPE
        *pdpe = (pdpe_t){0};
    }

    // Get the PDE
    pde = get_pde(page_base_address);
    if (!pde->present) {
        return;
    }

    // Get the PT physical address and check if it's in the buffer
    data_structure_physical_address = utils::patch_page_address(pde->pt_address);
    if (data_structure_physical_address >= buf_physical_address &&
        data_structure_physical_address < (buf_physical_address - buf_size)) {
        // If the PT is in the buffer, disable the PDE
        *pde = (pde_t){0};
    }

    // Get the PTE
    pte = get_pte(page_base_address);

    // Disable the PTE
    *pte = (pte_t){0};

    // Invalidate the page
    invalidate_page(page_base_address);
}

void influx::memory::paging_manager::init_user_process_paging(uint64_t pml4t_virtual_address) {
    pml4e_t *pml4t = (pml4e_t *)pml4t_virtual_address;

    pml4e_t recursive_pml4e = {.raw = 0};

    // Reset all other entries
    utils::memset(pml4t, 0, sizeof(pml4e_t) * (AMOUNT_OF_PAGE_TABLE_ENTRIES - 2));

    // Map the kernel inside the user process' paging
    *(pml4t + AMOUNT_OF_PAGE_TABLE_ENTRIES - 2) = *get_pml4e(HIGHER_HALF_KERNEL_OFFSET);

    // Recursive map of the paging structures in the last PDPE
    recursive_pml4e.address_placeholder =
        utils::patch_page_address_set_value(get_physical_address(pml4t_virtual_address)) &
        0xFFFFFFFFFF;
    recursive_pml4e.present = true;
    recursive_pml4e.read_write = READ_WRITE_ACCESS;
    *(pml4t + AMOUNT_OF_PAGE_TABLE_ENTRIES - 1) = recursive_pml4e;
}

void influx::memory::paging_manager::set_pte_permissions(uint64_t virtual_address,
                                                         protection_flags_t pflags,
                                                         bool user_access) {
    pml4e_t *pml4e = get_pml4e(virtual_address);
    pdpe_t *pdpe = get_pdpe(virtual_address);
    pde_t *pde = get_pde(virtual_address);
    pte *pte = get_pte(virtual_address);

    // If the PTE is present
    if (pml4e->present && pdpe->present && pde->present) {
        // If the protection flag are none, disable the PTE
        if (pflags == PROT_NONE) {
            pte->present = false;
        } else if (pflags & (PROT_READ | PROT_WRITE)) {
            pte->read_write = READ_WRITE_ACCESS;
        } else if (pflags & PROT_READ) {
            pte->read_write = READ_ONLY_ACCESS;
        }

        // If the PTE should be executable
        if (pflags & PROT_EXEC) {
            pte->no_execute = false;
        } else {
            pte->no_execute = true;
        }

        // If we need to allow user access to the page
        if (user_access) {
            pml4e->user_supervisor = SUPERVISOR_USER_ACCESS;
            pdpe->user_supervisor = SUPERVISOR_USER_ACCESS;
            pde->user_supervisor = SUPERVISOR_USER_ACCESS;
            pte->user_supervisor = SUPERVISOR_USER_ACCESS;
        } else {
            pte->user_supervisor = SUPERVISOR_ONLY_ACCESS;
        }

        // Invalidate the page to refresh it
        invalidate_page(virtual_address);
    }
}

bool influx::memory::paging_manager::allocate_structures_buffer() {
    uint64_t structures_mapping_buffer_physical_address =
        get_physical_address((uint64_t)_structures_mapping_temp_buffer);
    buffer_t structures_mapping_buffer{.ptr = (void *)_structures_mapping_temp_buffer,
                                       .size = PAGE_SIZE};

    // Allocate physical pages for the structures buffer
    int64_t structures_buffer_first_page =
        physical_allocator::alloc_consecutive_pages(STRUCTURES_BUFFER_SIZE / PAGE_SIZE);
    if (structures_buffer_first_page < 0) {
        return false;
    }

    // Allocate all pages for the structures buffer
    for (uint8_t i = 0; i < STRUCTURES_BUFFER_SIZE / PAGE_SIZE; i++) {
        if (!temp_map_page(STRUCTURES_BUFFER_ADDRESS + i * PAGE_SIZE, structures_mapping_buffer,
                           structures_mapping_buffer_physical_address,
                           structures_buffer_first_page + i)) {
            _structures_buffer = {.ptr = nullptr, .size = 0};
            return false;
        }

        // Set the page as R/W
        paging_manager::set_pte_permissions(STRUCTURES_BUFFER_ADDRESS + i * PAGE_SIZE,
                                            PROT_READ | PROT_WRITE);
    }

    // Init the buffer
    _structures_buffer = {.ptr = (void *)STRUCTURES_BUFFER_ADDRESS, .size = STRUCTURES_BUFFER_SIZE};

    return true;
}

void influx::memory::paging_manager::free_structures_buffer() {
    uint64_t structures_mapping_buffer_physical_address =
        get_physical_address((uint64_t)_structures_mapping_temp_buffer);

    // Unmap all pages for the structures buffer
    for (uint8_t i = 0; i < STRUCTURES_BUFFER_SIZE / PAGE_SIZE; i++) {
        unmap_temp_mapping(STRUCTURES_BUFFER_ADDRESS + i * PAGE_SIZE,
                           structures_mapping_buffer_physical_address, PAGE_SIZE);
    }
}

void influx::memory::paging_manager::invalidate_page(uint64_t page_base_virtual_address) {
    __asm__ __volatile__("invlpg [%0]" : : "r"(page_base_virtual_address) : "memory");
}