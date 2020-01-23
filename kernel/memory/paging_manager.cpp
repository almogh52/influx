#include <kernel/memory/paging_manager.h>

#include <kernel/memory/utils.h>
#include <stdint.h>

influx::memory::paging_manager::paging_manager(void *pml4) : _pml4((pml4e_t *)pml4) {}

pml4e_t *influx::memory::paging_manager::get_pml4e(uint64_t address) {
    // Get the entry index of the PML4 entry
    uint16_t pml4_entry_index = (uint16_t)utils::patch_page_entry_address(address >> 39);

    return _pml4 + pml4_entry_index;
}

pdpe_t *influx::memory::paging_manager::get_pdpe(uint64_t address) {
    // Get the PML4E that holds the address
    pml4e_t *pml4e = get_pml4e(address);
    pdpe_t *pdpt = (pdpe_t *)(pml4e->pdp_address & 0xFFFFFFFFFF000);

    // Get the entry index of the PDPT entry
    uint16_t pdpt_entry_index = (uint16_t)utils::patch_page_entry_address(address >> 30);

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
    uint16_t pdt_entry_index = (uint16_t)utils::patch_page_entry_address(address >> 21);

    // If the PDPE isn't present, return null
    if (!pdpe || !pdpe->present) {
        return nullptr;
    }

    // Get the PDT address
    pdt = (pde_t *)(pdpe->pd_address & 0xFFFFFFFFFF000);

    return pdt + pdt_entry_index;
}

pte_t *influx::memory::paging_manager::get_pte(uint64_t address) {
    // Get the PDE that holds the address
    pde_t *pde = get_pde(address);
    pte_t *pt = nullptr;

    // Get the entry index of the PT entry
    uint16_t pt_entry_index = (uint16_t)utils::patch_page_entry_address(address >> 12);

    // If the PDE isn't present, return null
    if (!pde || !pde->present) {
        return nullptr;
    }

    // Get the PT address
    pt = (pte_t *)(pde->pt_address & 0xFFFFFFFFFF000);

    return pt + pt_entry_index;
}

pdpe_t *influx::memory::paging_manager::create_pdpe(uint64_t address) {}