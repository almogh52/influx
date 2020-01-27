#include <kernel/memory/liballoc.h>
#include <kernel/memory/virtual_allocator.h>
#include <memory/paging.h>
#include <stdint.h>

uint32_t liballoc_mutex = 0;

extern "C" int liballoc_lock() {
    while (!__sync_bool_compare_and_swap(&liballoc_mutex, 0, 1)) {
        __sync_synchronize();
    }

    return 0;
};

extern "C" int liballoc_unlock() {
    __sync_synchronize();

    // Release the mutex
    liballoc_mutex = 0;
}

extern "C" void *liballoc_alloc(uint64_t amount_of_pages) {
    return influx::memory::virtual_allocator::allocate(amount_of_pages * PAGE_SIZE, PROT_READ | PROT_WRITE);
}

extern "C" int liballoc_free(void *ptr, uint64_t amount_of_pages) {
	influx::memory::virtual_allocator::free(ptr, amount_of_pages * PAGE_SIZE);

	return 0;
}