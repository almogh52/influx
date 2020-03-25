#include <kernel/vfs/vfs.h>

#include <kernel/fs/ext2/ext2.h>
#include <kernel/threading/lock_guard.h>

influx::vfs::vfs::vfs()
    : _log("VFS", console_color::green),
      _vnodes(vnode{.file_info = file{.inode = UINT64_MAX,
                                      .file_name = "",
                                      .type = file_type::unknown,
                                      .size = 0,
                                      .created = 0,
                                      .modified = 0,
                                      .accessed = 0},
                    .drive = drivers::ata::drive_slice(),
                    .amount_of_open_files = 0}) {}

bool influx::vfs::vfs::mount(influx::vfs::fs_type type, influx::vfs::path mount_path,
                             influx::drivers::ata::drive_slice drive) {
    threading::lock_guard lk(_mounts_mutex);
    filesystem* fs = nullptr;

    // Check nothing is mounted on mount path
    for (const auto& mount : _mounts) {
        if (mount.mount_path == mount_path) {
            _log("Drive already mounted on '%s'!\n", mount_path.string().c_str());
            return false;
        }
    }

    // Create the filesystem object for the drive
    _log("Creating filesystem with type %d..\n", type);
    switch (type) {
        case fs_type::ext2:
            fs = new fs::ext2(drive);
            break;

        default:
            _log("Unknown filesystem type: %d!\n", type);
            return false;
    }

    // Mount the filesystem in the drive
    _log("Mounting %s filesystem for drive %s in '%s'..\n", fs->name().c_str(),
         drive.drive_info().to_string().c_str(), mount_path.string().c_str());
    if (!fs->mount(mount_path)) {
        _log("Failed to mount %s filesystem in drive %s.\n", fs->name().c_str(),
             drive.drive_info().to_string().c_str());
    }

    // Add the new mount
    _mounts.push_back(fs_mount{.fs = fs, .mount_path = mount_path});

    _log("%s(drive: %s) filesystem was successfully mounted in '%s'.\n", fs->name().c_str(),
         drive.drive_info().to_string().c_str(), mount_path.string().c_str());

    return true;
}