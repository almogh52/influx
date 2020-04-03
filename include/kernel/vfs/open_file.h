#pragma once
#include <kernel/vfs/access_type.h>
#include <stdint.h>

namespace influx {
namespace vfs {
struct open_file {
    uint64_t vnode_index;
    uint64_t position;
    access_type access;

    inline bool operator!=(const open_file& f) const { return !(*this == f); }
    inline bool operator==(const open_file& f) const {
        return vnode_index == f.vnode_index && access == f.access;
    }
};
};  // namespace vfs
};  // namespace influx