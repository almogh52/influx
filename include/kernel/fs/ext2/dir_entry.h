#pragma once
#include <stdint.h>

namespace influx {
namespace fs {
enum class ext2_dir_entry_type : uint8_t {
    unknown = 0,
    regular = 1,
    directory = 2,
    character_device = 3,
    block_device = 4,
    fifo = 5,
    socket = 6,
    symbolic_link = 7
};

struct ext2_dir_entry {
    uint32_t inode;
    uint16_t size;
    uint8_t name_len;
    ext2_dir_entry_type type;
    char name[];
};
};  // namespace fs
};  // namespace influx