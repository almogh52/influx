#pragma once

#include <memory/paging.h>

namespace influx {
namespace memory {
class paging_manager {
   public:
    paging_manager(void *pml4);

    pml4e_t *get_pml4e(uint64_t address);
    pdpe_t *get_pdpe(uint64_t address);
    pde_t *get_pde(uint64_t address);
    pte_t *get_pte(uint64_t address);

    pdpe_t *alloc_pdpt(uint64_t pml4t_index, uint64_t alloc_address=0);
    pde_t *alloc_pdt(pdpe_t *pdpt, uint64_t pdpt_index, uint64_t alloc_address=0);
    pte_t *alloc_pt(pde_t *pdt, uint64_t pdt_index, uint64_t alloc_address=0);

   private:
    pml4e_t *_pml4;
};
};  // namespace memory
};  // namespace influx