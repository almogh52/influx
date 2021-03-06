#pragma once
#include <kernel/drivers/ata/slice.h>
#include <kernel/logger.h>
#include <kernel/structures/hash_map.h>
#include <kernel/structures/reference_wrapper.h>
#include <kernel/structures/unique_hash_map.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/mutex.h>
#include <kernel/threading/scheduler.h>
#include <kernel/tty/tty_manager.h>
#include <kernel/vfs/error.h>
#include <kernel/vfs/fs_mount.h>
#include <kernel/vfs/fs_type.h>
#include <kernel/vfs/open_file.h>
#include <kernel/vfs/open_flags.h>
#include <kernel/vfs/pipe_manager.h>
#include <kernel/vfs/seek_type.h>
#include <kernel/vfs/vnode.h>
#include <stddef.h>

namespace influx {
namespace vfs {
class vfs {
   public:
    vfs();

    bool mount(fs_type type, path mount_path, drivers::ata::drive_slice drive);

    int64_t open(const path& file_path, open_flags flags,
                 file_permissions permissions = {.raw = 0});
    int64_t close(size_t fd);

    int64_t stat(size_t fd, file_info& info);
    int64_t stat(const path& file_path, file_info& info);

    int64_t seek(size_t fd, int64_t offset, seek_type type);
    int64_t read(size_t fd, void* buf, size_t count);
    int64_t write(size_t fd, const void* buf, size_t count);

    int64_t mkdir(const path& dir_path, file_permissions permissions);
    int64_t get_dir_entries(size_t fd, structures::vector<dir_entry>& entries,
                            uint64_t dirent_buffer_size);

    int64_t unlink(const path& file_path);

    filesystem* get_filesystem(size_t fd);
    int64_t get_vnode_index(size_t fd);

    void fork_file_descriptors(structures::unique_hash_map<open_file>& file_descriptors);

    inline pipe_manager* pipe_handler() { return &_pipe_manager; };

    static uint64_t dirent_size_for_dir_entry(dir_entry& entry);

   private:
    logger _log;

    structures::vector<fs_mount> _mounts;
    threading::mutex _mounts_mutex;

    structures::unique_hash_map<vnode> _vnodes;
    structures::hash_map<uint64_t, path> _deleted_vnodes_paths;
    threading::mutex _vnodes_mutex;

    pipe_manager _pipe_manager;

    error get_open_file_for_fd(int64_t fd, open_file& file);

    error get_vnode_for_file(filesystem* fs, void* fs_file_data,
                             structures::pair<uint64_t, structures::reference_wrapper<vnode>>& vn);
    error create_vnode_for_file(
        filesystem* fs, void* fs_file_data, file_info file,
        structures::pair<uint64_t, structures::reference_wrapper<vnode>>& vn);

    filesystem* get_fs_for_file(const path& file_path);

    void close_open_file(const open_file& file);

    friend class influx::tty::tty_manager;
    friend class influx::threading::scheduler;
    friend class influx::vfs::pipe_manager;
};
};  // namespace vfs
};  // namespace influx