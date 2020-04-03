#include <kernel/memory/virtual_allocator.h>

#include <kernel/console/early_console.h>
#include <kernel/memory/paging_manager.h>
#include <kernel/memory/physical_allocator.h>
#include <kernel/memory/utils.h>
#include <memory/protection_flags.h>

void influx::memory::virtual_allocator::init(const boot_info_mem &mmap) {
    vma_node_t *current_node = nullptr;

    // Allocate the head of the vma list which is the entire VMA of the kernel
    _vma_list_head = alloc_vma_node({.base_addr = KERNEL_VMA_START,
                                     .size = KERNEL_VMA_SIZE,
                                     .protection_flags = 0x0,
                                     .allocated = false});

    // Insert the page of the initial VMA list
    insert_vma_region({.base_addr = VMA_LIST_INITIAL_ADDRESS,
                       .size = PAGE_SIZE,
                       .protection_flags = PROT_READ | PROT_WRITE,
                       .allocated = true});

    // For each kernel memory entry, set it's region as allocated
    for (uint32_t i = 0; i < mmap.entry_count; i++) {
        if (mmap.entries[i].type == KERNEL) {
            insert_vma_region({.base_addr = mmap.entries[i].base_addr + HIGHER_HALF_KERNEL_OFFSET,
                               .size = mmap.entries[i].size,
                               .protection_flags = PROT_READ | PROT_WRITE | PROT_EXEC,
                               .allocated = true});
        }
    }

    // Add the physical allocator bitmap VMA region
    insert_vma_region(physical_allocator::get_bitmap_region());

    // Add the paging manager structures VMA region
    insert_vma_region(paging_manager::get_bitmap_region());

    // Add the early console VMA region
    insert_vma_region(early_console::get_vma_region());

    // Add the initial page of VMA list
    insert_vma_region({.base_addr = VMA_LIST_INITIAL_ADDRESS,
                       .size = PAGE_SIZE,
                       .protection_flags = PROT_READ | PROT_WRITE,
                       .allocated = true});

    // Invalidate all non-tracked memory
    current_node = _vma_list_head;
    while (current_node != nullptr) {
        // If the region isn't allocated and it's in the early memory region, invalidate it
        if (current_node->value().allocated == false) {
            for (uint64_t i = current_node->value().base_addr % PAGE_SIZE == 0
                                  ? current_node->value().base_addr
                                  : current_node->value().base_addr + PAGE_SIZE -
                                        (current_node->value().base_addr % PAGE_SIZE);
                 i < (current_node->value().base_addr + current_node->value().size) &&
                 i < (HIGHER_HALF_KERNEL_OFFSET + EARLY_MEMORY_SIZE);
                 i += PAGE_SIZE) {
                paging_manager::set_pte_permissions(i, PROT_NONE);
            }
        }

        // Move to the next node
        current_node = current_node->next();
    }
}

void *influx::memory::virtual_allocator::allocate(uint64_t size, protection_flags_t pflags,
                                                  int64_t physical_page_index) {
    if (size % PAGE_SIZE == 0) {
        vma_region_t region = find_free_region(size, pflags);

        return allocate(region, physical_page_index);
    } else {
        // TODO: throw exception

        return nullptr;
    }
}

void influx::memory::virtual_allocator::free(void *ptr, uint64_t size) {
    uint64_t page_physical_address = 0;

    if (size % PAGE_SIZE == 0 && (uint64_t)ptr % PAGE_SIZE == 0) {
        // Free the VMA region
        if (!free_vma_region({.base_addr = (uint64_t)ptr,
                              .size = size,
                              .protection_flags = 0,
                              .allocated = true})) {
            // TODO: throw exception
        }

        // Free all physical pages and mapping
        for (uint64_t i = 0; i < size / PAGE_SIZE; i++) {
            // Get the physical address of the page and free it if available
            page_physical_address =
                paging_manager::get_physical_address((uint64_t)ptr + i * PAGE_SIZE);
            if (page_physical_address != 0) {
                physical_allocator::free_page(page_physical_address);
            }

            // Free the mapping to the page
            paging_manager::unmap_page((uint64_t)ptr + i * PAGE_SIZE);
        }
    } else {
        // TODO: throw exception
    }
}

influx::memory::vma_node_t *influx::memory::virtual_allocator::alloc_vma_node(
    vma_region_t region, bool insert_new_region) {
    void *vma_node_ptr = nullptr;
    vma_node_t *vma_node = nullptr;

    vma_region_t vma_list_page_region = {
        .base_addr = 0, .size = 0, .protection_flags = 0, .allocated = false};

    // Check if it's the initial alloc
    if (_current_vma_list_page.ptr == nullptr) {
        vma_list_page_region = {.base_addr = VMA_LIST_INITIAL_ADDRESS,
                                .size = PAGE_SIZE,
                                .protection_flags = PROT_READ | PROT_WRITE,
                                .allocated = true};
    } else if (_current_vma_list_page.size < sizeof(vma_node_t)) {
        // Find a free region for the VMA list new page
        vma_list_page_region = find_free_region(PAGE_SIZE, PROT_READ | PROT_WRITE, region);
    }

    // If a new page for the VMA list is needed, allocate it
    if (vma_list_page_region.size > 0) {
        // Allocate and map a VMA list page
        if (!paging_manager::map_page(vma_list_page_region.base_addr)) {
            return nullptr;
        }

        // Set the page as R/W
        paging_manager::set_pte_permissions(vma_list_page_region.base_addr, PROT_READ | PROT_WRITE);

        // Set the buffer
        _current_vma_list_page = {.ptr = (void *)vma_list_page_region.base_addr,
                                  .size = vma_list_page_region.size};

        // Set the region as allocated in the VMA list
        if (_vma_list_head != nullptr && insert_new_region) {
            insert_vma_region(vma_list_page_region);
        } else if (!insert_new_region) {
            _vma_list_pending_new_region = vma_list_page_region;
        }
    }

    // If there is still not enough memory for VMA node return null
    if (_current_vma_list_page.size < sizeof(vma_node_t)) {
        return nullptr;
    }

    // Allocate the node
    vma_node_ptr = _current_vma_list_page.ptr;
    utils::memset(vma_node_ptr, 0, sizeof(vma_node_t));

    // Initialize the object
    vma_node = (vma_node_t *)vma_node_ptr;
    *vma_node = vma_node_t(region);

    // Increase the pointer and reduce the size
    _current_vma_list_page.ptr =
        (void *)((uint8_t *)_current_vma_list_page.ptr + sizeof(vma_node_t));
    _current_vma_list_page.size -= sizeof(vma_node_t);

    return vma_node;
}

void influx::memory::virtual_allocator::insert_vma_region(vma_region_t region) {
    vma_node_t *new_node = nullptr, *remainder_new_node = nullptr;
    vma_node_t *vma_region_container_node = nullptr;

    vma_region_t vma_list_pending_new_region_copy;

    // Try to find if the new region is contained in another existing region
    if ((vma_region_container_node =
             _vma_list_head->find_node(region.base_addr, &address_in_vma_region)) != nullptr) {
        // If the new region starts in the same address as the container node
        if (vma_region_container_node->value().base_addr == region.base_addr) {
            // If the 2 regions has the same size, just swap their values
            if (vma_region_container_node->value().size == region.size) {
                vma_region_container_node->value() = region;
            } else {
                // Allocate the new node
                new_node = alloc_vma_node(region, false);

                // Resize the container node
                vma_region_container_node->value().base_addr += region.size;
                vma_region_container_node->value().size -= region.size;

                // Insert it to the list
                if (vma_region_container_node->prev() != nullptr) {
                    vma_region_container_node->prev()->insert_next(new_node);
                } else {
                    _vma_list_head = _vma_list_head->insert(new_node);
                }
            }
        } else {
            // If the container ends where the new region ends
            if (vma_region_container_node->value().base_addr +
                    vma_region_container_node->value().size ==
                region.base_addr + region.size) {
                // Allocate the new node
                new_node = alloc_vma_node(region, false);

                // Resize the container node
                vma_region_container_node->value().size -= region.size;

                // Insert it to the list
                vma_region_container_node->insert_next(new_node);
            } else {
                // Allocate the new node
                new_node = alloc_vma_node(region, false);

                // Allocate the remainder node
                remainder_new_node = alloc_vma_node(
                    {.base_addr = region.base_addr + region.size,
                     .size = (vma_region_container_node->value().base_addr +
                              vma_region_container_node->value().size) -
                             (region.base_addr + region.size),
                     .protection_flags = vma_region_container_node->value().protection_flags,
                     .allocated = vma_region_container_node->value().allocated},
                    false);

                // Resize the container node
                vma_region_container_node->value().size =
                    region.base_addr - vma_region_container_node->value().base_addr;

                // Insert it to the list
                vma_region_container_node->insert_next(new_node);

                // Insert a new region after the new node
                new_node->insert_next(remainder_new_node);
            }
        }
    } else {
        // Allocate the new node
        new_node = alloc_vma_node(region, false);

        // If not in any of the other regions just add the region
        _vma_list_head = _vma_list_head->insert(new_node);
    }

    // If a new node was inserted
    if (new_node) {
        // Check for VMA node combination
        check_for_vma_node_combination(new_node);
    }

    // If there is a new pending VMA list page, insert it
    if (_vma_list_pending_new_region.size > 0) {
        vma_list_pending_new_region_copy = _vma_list_pending_new_region;
        _vma_list_pending_new_region = {
            .base_addr = 0, .size = 0, .protection_flags = 0, .allocated = false};
        insert_vma_region(vma_list_pending_new_region_copy);
    }
}

bool influx::memory::virtual_allocator::free_vma_region(vma_region_t region) {
    vma_node_t *new_node = nullptr, *remainder_new_node = nullptr;
    vma_node_t *vma_region_container_node = nullptr;

    vma_region_t vma_list_pending_new_region_copy;

    // If there is a node that contains the region
    if ((vma_region_container_node =
             _vma_list_head->find_node(region.base_addr, &address_in_vma_region)) != nullptr &&
        vma_region_container_node->value().allocated == true) {
        // If the container and the region starts in the same address
        if (vma_region_container_node->value().base_addr == region.base_addr) {
            // If they have the size as well
            if (vma_region_container_node->value().size == region.size) {
                // Free the region
                vma_region_container_node->value().allocated = false;
                vma_region_container_node->value().protection_flags = 0;
            } else {
                // Allocate a node for the remainder of the allocated region and insert it
                new_node = alloc_vma_node(
                    {.base_addr = region.base_addr + region.size,
                     .size = vma_region_container_node->value().size - region.size,
                     .protection_flags = vma_region_container_node->value().protection_flags,
                     .allocated = true},
                    false);
                vma_region_container_node->insert_next(new_node);

                // Resize the container
                vma_region_container_node->value().size = region.size;

                // Free the container region
                vma_region_container_node->value().allocated = false;
                vma_region_container_node->value().protection_flags = 0;
            }
        } else {
            // If the container and the region ends are the same
            if ((vma_region_container_node->value().base_addr +
                 vma_region_container_node->value().size) == (region.base_addr + region.size)) {
                // Resize the container region
                vma_region_container_node->value().size -= region.size;

                // Allocate a node for the new free region
                new_node = alloc_vma_node({.base_addr = region.base_addr,
                                           .size = region.size,
                                           .protection_flags = 0,
                                           .allocated = false},
                                          false);
                vma_region_container_node->insert_next(new_node);
            } else {
                // Resize the container region
                vma_region_container_node->value().size =
                    vma_region_container_node->value().base_addr - region.base_addr;

                // Allocate a noder for the remainder region
                remainder_new_node = alloc_vma_node(
                    {.base_addr = region.base_addr + region.size,
                     .size = (vma_region_container_node->value().base_addr +
                              vma_region_container_node->value().size) -
                             (region.base_addr + region.size),
                     .protection_flags = vma_region_container_node->value().protection_flags,
                     .allocated = true},
                    false);

                // Allocate a node for the new free region
                new_node = alloc_vma_node({.base_addr = region.base_addr,
                                           .size = region.size,
                                           .protection_flags = 0,
                                           .allocated = false},
                                          false);
                vma_region_container_node->insert_next(new_node);

                // Set the remainder VMA region node as the next node for the new node
                new_node->insert_next(remainder_new_node);
            }
        }

        // Check for VMA node combination for the container
        check_for_vma_node_combination(vma_region_container_node);

        // If a new node was allocated
        if (new_node) {
            // Check for VMA node combination
            check_for_vma_node_combination(new_node);
        }

        // If a remainder node was allocated
        if (remainder_new_node) {
            // Check for VMA node combination
            check_for_vma_node_combination(remainder_new_node);
        }

        // If there is a new pending VMA list page, insert it
        if (_vma_list_pending_new_region.size > 0) {
            vma_list_pending_new_region_copy = _vma_list_pending_new_region;
            _vma_list_pending_new_region = {
                .base_addr = 0, .size = 0, .protection_flags = 0, .allocated = false};
            insert_vma_region(vma_list_pending_new_region_copy);
        }

        return true;
    }

    // If there is a new pending VMA list page, insert it
    if (_vma_list_pending_new_region.size > 0) {
        insert_vma_region(_vma_list_pending_new_region);
        _vma_list_pending_new_region = {
            .base_addr = 0, .size = 0, .protection_flags = 0, .allocated = false};
    }

    return false;
}

void influx::memory::virtual_allocator::check_for_vma_node_combination(
    influx::memory::vma_node_t *node) {
    // If the previous node and the node has the same allocated status and protection flags,
    // combine them
    if (node->prev() != nullptr && node->prev()->value().allocated == node->value().allocated &&
        node->prev()->value().protection_flags == node->value().protection_flags) {
        // Increase the size of the previous node
        node->prev()->value().size += node->value().size;

        // Set the node as the previous node
        node = node->prev();

        // Delete the node
        _vma_list_head = _vma_list_head->remove(node->next());
    }

    // If the next node and the node has the same allocated status and protection flags,
    // combine them
    if (node->next() != nullptr && node->next()->value().allocated == node->value().allocated &&
        node->next()->value().protection_flags == node->value().protection_flags) {
        // Increase the size of the current node
        node->value().size += node->next()->value().size;

        // Delete the next node
        _vma_list_head = _vma_list_head->remove(node->next());
    }
}

bool influx::memory::virtual_allocator::address_in_vma_region(vma_region_t &region,
                                                              uint64_t &address) {
    return address >= region.base_addr && address < (region.base_addr + region.size);
}

vma_region_t influx::memory::virtual_allocator::find_free_region(uint64_t size,
                                                                 protection_flags_t pflags,
                                                                 vma_region ignore_region) {
    vma_node_t *current_node = _vma_list_head;

    // While we didn't reach the end of the list and the current node cannot fit the new region
    while (
        current_node != nullptr &&
        (current_node->value().size <
             size + (current_node->value().base_addr % PAGE_SIZE
                         ? PAGE_SIZE - (current_node->value().base_addr % PAGE_SIZE)
                         : 0) ||
         current_node->value().allocated == true ||
         (current_node->value().base_addr == ignore_region.base_addr &&
          (current_node->value().size - ignore_region.size) <
              size + ((ignore_region.base_addr + ignore_region.size) % PAGE_SIZE
                          ? PAGE_SIZE - ((ignore_region.base_addr + ignore_region.size) % PAGE_SIZE)
                          : 0)))) {
        current_node = current_node->next();
    }

    // If a node wasn't found, return empty region
    if (!current_node) {
        return {.base_addr = 0, .size = 0, .protection_flags = 0, .allocated = false};
    } else {
        return {.base_addr =
                    current_node->value().base_addr == ignore_region.base_addr
                        ? ((ignore_region.base_addr + ignore_region.size) +
                           ((ignore_region.base_addr + ignore_region.size) % PAGE_SIZE
                                ? PAGE_SIZE -
                                      ((ignore_region.base_addr + ignore_region.size) % PAGE_SIZE)
                                : 0))
                        : (current_node->value().base_addr +
                           (current_node->value().base_addr % PAGE_SIZE
                                ? PAGE_SIZE - (current_node->value().base_addr % PAGE_SIZE)
                                : 0)),
                .size = size,
                .protection_flags = pflags,
                .allocated = true};
    }
}

void *influx::memory::virtual_allocator::allocate(vma_region_t region,
                                                  int64_t physical_page_index) {
    // If a region wasn't found
    if (region.size == 0) {
        return nullptr;
    } else {
        // Map pages
        // TODO: Swap this with page-fault exception
        for (uint64_t i = 0; i < region.size / PAGE_SIZE; i++) {
            if (!paging_manager::map_page(
                    region.base_addr + i * PAGE_SIZE,
                    physical_page_index == -1 ? physical_page_index : physical_page_index + i)) {
                // Revert all page maps
                for (uint64_t j = 0; j < i; i++) {
                    paging_manager::unmap_page(region.base_addr + j * PAGE_SIZE);
                }

                return nullptr;
            }
            paging_manager::set_pte_permissions(region.base_addr + i * PAGE_SIZE,
                                                region.protection_flags);
        }

        // Insert the VMA region
        insert_vma_region(region);

        return (void *)region.base_addr;
    }
}