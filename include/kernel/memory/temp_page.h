#pragma once

#include <memory/paging.h>
#include <stdint.h>

namespace influx {
namespace memory {
struct temp_page {
    uint64_t page_index = 0;
    void *ptr = nullptr;
    char buffer[PAGE_SIZE];
};
};  // namespace memory
};  // namespace influx