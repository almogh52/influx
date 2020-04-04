#pragma once
#include <kernel/vfs/open_flags.h>
#include <stdint.h>

namespace influx {
namespace vfs {
struct open_file {
    uint64_t vnode_index;
    uint64_t position;
    open_flags flags;

    inline bool operator!=(const open_file& f) const { return !(*this == f); }
    inline bool operator==(const open_file& f) const {
        return vnode_index == f.vnode_index && flags == f.flags;
    }
};
};  // namespace vfs
};  // namespace influx