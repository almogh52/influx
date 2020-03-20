#pragma once
#include <kernel/drivers/ata/slice.h>
#include <kernel/logger.h>
#include <kernel/structures/unique_hash_map.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/mutex.h>
#include <kernel/vfs/fs_mount.h>
#include <kernel/vfs/fs_type.h>
#include <kernel/vfs/vnode.h>

namespace influx {
namespace vfs {
class vfs {
   public:
    vfs();

    bool mount(fs_type type, path mount_path, drivers::ata::drive_slice drive);

   private:
    logger _log;

    structures::vector<fs_mount> _mounts;
    threading::mutex _mounts_mutex;

    structures::unique_hash_map<vnode> _vnodes;
    threading::mutex _vnodes_mutex;
};
};  // namespace vfs
};  // namespace influx