#include <kernel/kernel.h>
#include <kernel/memory/liballoc.h>
#include <kernel/memory/virtual_allocator.h>
#include <kernel/threading/mutex.h>
#include <kernel/threading/scheduler.h>
#include <kernel/threading/scheduler_started.h>
#include <memory/paging.h>
#include <stdint.h>

uint32_t liballoc_early_mutex = 0;
influx::threading::mutex liballoc_mutex;

extern "C" int liballoc_lock() {
    // If the scheduler has started
    if (influx::threading::scheduler_started) {
        liballoc_mutex.lock();
    } else {
        while (!__sync_bool_compare_and_swap(&liballoc_early_mutex, 0, 1))
            ;
        __sync_synchronize();
    }

    return 0;
};

extern "C" int liballoc_unlock() {
    // If the scheduler has started
    if (influx::threading::scheduler_started) {
        liballoc_mutex.unlock();
    } else {
        // Release the mutex
        __sync_synchronize();
        liballoc_early_mutex = 0;
    }

    return 0;
}

extern "C" void *liballoc_alloc(uint64_t amount_of_pages) {
    return influx::memory::virtual_allocator::allocate(amount_of_pages * PAGE_SIZE,
                                                       PROT_READ | PROT_WRITE);
}

extern "C" int liballoc_free(void *ptr, uint64_t amount_of_pages) {
    influx::memory::virtual_allocator::free(ptr, amount_of_pages * PAGE_SIZE);

    return 0;
}