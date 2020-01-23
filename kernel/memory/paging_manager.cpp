
#include <kernel/memory/paging_manager.h>

#include <kernel/memory/utils.h>
#include <stdint.h>

influx::memory::paging_manager::paging_manager(void *pml4) : _pml4((pml4e_t *)pml4) {}

pml4e_t *influx::memory::paging_manager::get_pml4e(uint64_t address) {
    // Get the entry index of the PML4 entry
    uint16_t pml4_entry_index = (uint16_t)utils::get_page_entry_index(address >> 39);

    return _pml4 + pml4_entry_index;
}

pdpe_t *influx::memory::paging_manager::get_pdpe(uint64_t address) {
    // Get the PML4E that holds the address
    pml4e_t *pml4e = get_pml4e(address);
    pdpe_t *pdpt = (pdpe_t *)utils::patch_page_address(pml4e->pdp_address);

    // Get the entry index of the PDPT entry
    uint16_t pdpt_entry_index = (uint16_t)utils::get_page_entry_index(address >> 30);

    // If the PML4E isn't present, return null
    if (!pml4e->present) {
        return nullptr;
    }

    return pdpt + pdpt_entry_index;
}

pde_t *influx::memory::paging_manager::get_pde(uint64_t address) {
    // Get the PDPE that holds the address
    pdpe_t *pdpe = get_pdpe(address);
    pde_t *pdt = nullptr;

    // Get the entry index of the PDT entry
    uint16_t pdt_entry_index = (uint16_t)utils::get_page_entry_index(address >> 21);

    // If the PDPE isn't present, return null
    if (!pdpe || !pdpe->present) {
        return nullptr;
    }

    // Get the PDT address
    pdt = (pde_t *)utils::patch_page_address(pdpe->pd_address);

    return pdt + pdt_entry_index;
}

pte_t *influx::memory::paging_manager::get_pte(uint64_t address) {
    // Get the PDE that holds the address
    pde_t *pde = get_pde(address);
    pte_t *pt = nullptr;

    // Get the entry index of the PT entry
    uint16_t pt_entry_index = (uint16_t)utils::get_page_entry_index(address >> 12);

    // If the PDE isn't present, return null
    if (!pde || !pde->present) {
        return nullptr;
    }

    // Get the PT address
    pt = (pte_t *)utils::patch_page_address(pde->pt_address);

    return pt + pt_entry_index;
}

pdpe_t *influx::memory::paging_manager::alloc_pdpt(uint64_t pml4t_index, uint64_t alloc_address) {
    pdpe_t *pdpt = nullptr;

    // If there isn't already a PDPT for the PML4 index
    if ((pdpt = get_pdpe(pml4t_index << 39)) != nullptr) {
        // If an alloc address was specified
        if (alloc_address != 0) {
            pdpt = (pdpe_t *)alloc_address;
        }

        // If the PDPT was allocated
        if (pdpt) {
            // Reset all entries
            for (uint16_t i = 0; i < AMOUNT_OF_PAGE_TABLE_ENTRIES; i++) {
                *(pdpt + i) = (pdpe_t){0};
            }

            // Set the PDPT for the PML4E
            get_pml4e(pml4t_index)->address_placeholder =
                utils::patch_page_address_set_value((uint64_t)pdpt) & 0xFFFFFFFFFF;
        }
    }

    return pdpt;
}

pde_t *influx::memory::paging_manager::alloc_pdt(pdpe_t *pdpt, uint64_t pdpt_index,
                                                 uint64_t alloc_address) {
    pde_t *pdt = nullptr;

    // If there is already a PDT associated with the PDPE
    if ((pdpt + pdpt_index)->present) {
        // Get the real address of the PDT
        // TODO: Get the address from the virtual allocator
        pdt = (pde_t *)utils::patch_page_address((pdpt + pdpt_index)->pd_address);
    } else {
        // If an alloc address was specified
        if (alloc_address != 0) {
            pdt = (pde_t *)alloc_address;
        }

        // If the PDT was allocated
        if (pdt) {
            // Reset all entries
            for (uint16_t i = 0; i < AMOUNT_OF_PAGE_TABLE_ENTRIES; i++) {
                *(pdt + i) = (pde_t){0};
            }

            // Set the PDT for the PDPE
            (pdpt + pdpt_index)->address_placeholder =
                utils::patch_page_address_set_value((uint64_t)pdt) & 0xFFFFFFFFFF;
        }
    }

    return pdt;
}

pte_t *influx::memory::paging_manager::alloc_pt(pde_t *pdt, uint64_t pdt_index,
                                                uint64_t alloc_address) {
    pte_t *pt = nullptr;

    // If there is already a PT associated with the PDE
    if ((pdt + pdt_index)->present) {
        // Get the real address of the PT
        // TODO: Get the address from the virtual allocator
        pt = (pte_t *)utils::patch_page_address((pdt + pdt_index)->pt_address);
    } else {
        // If an alloc address was specified
        if (alloc_address != 0) {
            pt = (pte_t *)alloc_address;
        }

        // If the PT was allocated
        if (pt) {
            // Reset all entries
            for (uint16_t i = 0; i < AMOUNT_OF_PAGE_TABLE_ENTRIES; i++) {
                *(pt + i) = (pte_t){0};
            }

            // Set the PT for the PDE
            (pdt + pdt_index)->address_placeholder =
                utils::patch_page_address_set_value((uint64_t)pt) & 0xFFFFFFFFFF;
        }
    }

    return pt;
}