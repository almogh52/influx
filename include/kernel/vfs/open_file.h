#pragma once
#include <kernel/vfs/access_type.h>
#include <stdint.h>

namespace influx {
namespace vfs {
struct open_file {
    uint64_t vnode_index;
    uint64_t position;
    access_type access_type;
};
};  // namespace vfs
};  // namespace influx