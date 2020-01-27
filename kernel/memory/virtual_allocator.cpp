#include <kernel/memory/paging_manager.h>
#include <kernel/memory/physical_allocator.h>
#include <kernel/memory/utils.h>
#include <kernel/memory/virtual_allocator.h>
#include <memory/protection_flags.h>

void influx::memory::virtual_allocator::init(const boot_info_mem &mmap) {
    // Allocate the head of the vma list which is the entire VMA of the kernel
    _vma_list_head = alloc_vma_node({.base_addr = KERNEL_VMA_START,
                                     .size = KERNEL_VMA_SIZE,
                                     .protection_flags = 0x0,
                                     .allocated = false});

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

    // Add the initial page of VMA list
    insert_vma_region({.base_addr = (uint64_t)_current_vma_list_page.ptr,
                       .size = PAGE_SIZE,
                       .protection_flags = PROT_READ | PROT_WRITE,
                       .allocated = true});
}

void *influx::memory::virtual_allocator::allocate(uint64_t size, protection_flags_t pflags) {
    uint64_t aligned_size = size + (size % PAGE_SIZE ? 1 : 0);

    vma_region_t region = find_free_region(size, pflags);

    // If a region wasn't found
    if (region.size == 0) {
        return nullptr;
    } else {
        // Insert the VMA region
        insert_vma_region(region);

        return (void *)region.base_addr;
    }
}

influx::memory::vma_node_t *influx::memory::virtual_allocator::alloc_vma_node(vma_region_t region) {
    void *vma_node_ptr = nullptr;
    vma_node_t *vma_node = nullptr;

    // Check if it's the initial alloc
    if (_current_vma_list_page.ptr == nullptr) {
        // Allocate and map the initial VMA list address
        paging_manager::map_page(VMA_LIST_INITIAL_ADDRESS);

        // Set the page as R/W
        paging_manager::set_pte_permissions(VMA_LIST_INITIAL_ADDRESS, PROT_READ | PROT_WRITE);

        // Set the buffer
        _current_vma_list_page = {.ptr = (void *)VMA_LIST_INITIAL_ADDRESS, .size = PAGE_SIZE};
    } else if (_current_vma_list_page.size < sizeof(vma_node_t)) {
        // TODO: Find a virtual address to continue the VMA list in
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
                new_node = alloc_vma_node(region);

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
                new_node = alloc_vma_node(region);

                // Resize the container node
                vma_region_container_node->value().size -= region.size;

                // Insert it to the list
                vma_region_container_node->insert_next(new_node);
            } else {
                // Allocate the new node
                new_node = alloc_vma_node(region);

                // Allocate the remainder node
                remainder_new_node = alloc_vma_node(
                    {.base_addr = region.base_addr + region.size,
                     .size = (vma_region_container_node->value().base_addr +
                              vma_region_container_node->value().size) -
                             (region.base_addr + region.size),
                     .protection_flags = vma_region_container_node->value().protection_flags,
                     .allocated = vma_region_container_node->value().allocated});

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
        new_node = alloc_vma_node(region);

        // If not in any of the other regions just add the region
        _vma_list_head = _vma_list_head->insert(new_node);
    }

    // If a new node was inserted
    if (new_node) {
        // Check for VMA node combination
        check_for_vma_node_combination(new_node);
    }
}

void influx::memory::virtual_allocator::free_vma_region(vma_region_t region) {
    vma_node_t *new_node = nullptr, *remainder_new_node = nullptr;
    vma_node_t *vma_region_container_node = nullptr;

    // If there is a node that contains the region
    if ((vma_region_container_node =
             _vma_list_head->find_node(region.base_addr, &address_in_vma_region)) != nullptr) {
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
                     .allocated = true});
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
                                           .allocated = false});
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
                     .allocated = true});

                // Allocate a node for the new free region
                new_node = alloc_vma_node({.base_addr = region.base_addr,
                                           .size = region.size,
                                           .protection_flags = 0,
                                           .allocated = false});
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
    }
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
                                                                 protection_flags_t pflags) {
    vma_node_t *current_node = _vma_list_head;

    // While we didn't reach the end of the list and the current node cannot fit the new region
    while (current_node != nullptr && current_node->value().size >= size &&
           current_node->value().allocated == false) {
        current_node = current_node->next();
    }

    // If a node wasn't found, return empty region
    if (!current_node) {
        return {.base_addr = 0, .size = 0, .protection_flags = 0, .allocated = false};
    } else {
        return {.base_addr = current_node->value().base_addr,
                .size = size,
                .protection_flags = pflags,
                .allocated = true};
    }
}