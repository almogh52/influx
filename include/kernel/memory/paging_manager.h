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

    pdpe_t *create_pdpe(uint64_t address);

   private:
    pml4e_t *_pml4;
};
};  // namespace memory
};  // namespace influx