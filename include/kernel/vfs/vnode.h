#pragma once
#include <kernel/drivers/ata/slice.h>
#include <kernel/vfs/file.h>
#include <stdint.h>

namespace influx {
namespace vfs {
struct vnode {
    file file_info;
    drivers::ata::drive_slice drive;
    uint64_t amount_of_open_files;
};
};  // namespace vfs
};  // namespace influx