#pragma once
#include <kernel/vfs/open_file.h>
#include <stdint.h>

namespace influx {
namespace vfs {
struct file_descriptor {
    uint64_t open_file_index;

    inline bool operator!=(const file_descriptor& fd) const { return !(*this == fd); }
    inline bool operator==(const file_descriptor& fd) const {
        return open_file_index == fd.open_file_index;
    }
};
};  // namespace vfs
};  // namespace influx