#pragma once
#include <kernel/vfs/filesystem.h>
#include <kernel/vfs/path.h>

namespace influx {
namespace vfs {
struct fs_mount {
    filesystem *fs;
    path mount_path;
};
};  // namespace vfs
};  // namespace influx