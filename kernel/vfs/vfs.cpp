#include <kernel/vfs/vfs.h>

#include <kernel/algorithm.h>
#include <kernel/fs/ext2/ext2.h>
#include <kernel/kernel.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/scheduler.h>
#include <kernel/threading/unique_lock.h>
#include <kernel/time/time_manager.h>

influx::vfs::vfs::vfs() : _log("VFS", console_color::green) {}

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
        return false;
    }

    // Add the new mount
    _mounts.push_back(fs_mount{.fs = fs, .mount_path = mount_path});

    _log("%s(drive: %s) filesystem was successfully mounted in '%s'.\n", fs->name().c_str(),
         drive.drive_info().to_string().c_str(), mount_path.string().c_str());

    return true;
}

int64_t influx::vfs::vfs::open(const influx::vfs::path& file_path,
                               influx::vfs::access_type access) {
    filesystem* fs = get_fs_for_file(file_path);
    void* fs_file_data = nullptr;

    error err = error::success;
    file_info file;

    structures::pair<uint64_t, structures::reference_wrapper<vnode>> vn(0, _vnodes.empty_item());

    // If no filesystem contains this file path
    if (fs == nullptr) {
        return error::file_not_found;
    }

    // If the file wasn't found in the filesystem
    if ((fs_file_data = fs->get_fs_file_data(file_path)) == nullptr) {
        return error::file_not_found;
    }

    // Read file info from filesystem
    if ((err = fs->get_file_info(fs_file_data, file)) != error::success) {
        delete (uint32_t*)fs_file_data;
        return err;
    }

    // Verify file and access permissions
    if (((access & access_type::read) && !file.permissions.read) ||
        ((access & access_type::write || access & access_type::append) &&
         !file.permissions.write)) {
        delete (uint32_t*)fs_file_data;
        return error::insufficient_permissions;
    }

    // Try to find the vnode for the file
    if (((err = get_vnode_for_file(fs, fs_file_data, vn)) == error::vnode_not_found &&
         (err = create_vnode_for_file(fs, fs_file_data, file, vn)) != error::success) ||
        err != error::success) {
        delete (uint32_t*)fs_file_data;
        return err;
    }

    // Lock the file mutex
    threading::lock_guard lk(vn.second->file_mutex);

    // Increase the amount of open files
    vn.second->amount_of_open_files++;

    // Create the file descriptor and return it
    return kernel::scheduler()->add_file_descriptor(
        open_file{.vnode_index = vn.first, .position = 0, .access = access});
}

int64_t influx::vfs::vfs::read(int64_t fd, void* buf, size_t count) {
    open_file file;
    vnode vn_cpy;
    size_t amount_read = 0;

    error err;

    threading::unique_lock vnodes_lk(_vnodes_mutex);

    // Try to get the open file object
    if ((err = get_open_file_for_fd(fd, file)) != error::success) {
        return err;
    }

    // Create a copy of the vnode
    vn_cpy = _vnodes[file.vnode_index];

    // Check access for the file
    if (!(file.access & access_type::read)) {
        return error::invalid_file_access;
    }

    // Lock the file mutex
    threading::lock_guard file_lk(_vnodes[file.vnode_index].file_mutex);
    vnodes_lk.unlock();

    // Read the file
    if ((err = vn_cpy.fs->read(vn_cpy.fs_data, (char*)buf,
                               algorithm::min<size_t>(count, vn_cpy.file.size - file.position),
                               file.position, amount_read)) != error::success) {
        return err;
    }

    // Update file position
    file.position += amount_read;
    kernel::scheduler()->update_file_descriptor(fd, file);

    // Update last accessed time
    vnodes_lk.lock();
    _vnodes[file.vnode_index].file.accessed = kernel::time_manager()->unix_timestamp();

    return amount_read;
}

influx::vfs::error influx::vfs::vfs::get_open_file_for_fd(int64_t fd,
                                                          influx::vfs::open_file& file) {
    return kernel::scheduler()->get_file_descriptor(fd, file);
}

influx::vfs::error influx::vfs::vfs::get_vnode_for_file(
    influx::vfs::filesystem* fs, void* fs_file_data,
    influx::structures::pair<uint64_t, influx::structures::reference_wrapper<influx::vfs::vnode>>&
        vn) {
    threading::lock_guard lk(_vnodes_mutex);

    // For each vnode pair, check if it
    for (auto& vnode_pair : _vnodes) {
        // If the vnode fs file data matches return it's index
        if (fs->compare_fs_file_data(fs_file_data, vnode_pair.second.fs_data)) {
            vn = structures::pair<uint64_t, structures::reference_wrapper<vnode>>(
                vnode_pair.first, _vnodes[vnode_pair.first]);
            return error::success;
        }
    }

    return error::vnode_not_found;
}

influx::vfs::error influx::vfs::vfs::create_vnode_for_file(
    influx::vfs::filesystem* fs, void* fs_file_data, influx::vfs::file_info file,
    influx::structures::pair<uint64_t, influx::structures::reference_wrapper<influx::vfs::vnode>>&
        vn) {
    threading::lock_guard lk(_vnodes_mutex);

    structures::pair<structures::unique_hash_map<vnode>::iterator, bool> vnode_pair(_vnodes.end(),
                                                                                    false);

    // Insert vnode
    if ((vnode_pair = _vnodes.emplace_unique(file, fs, fs_file_data)).second == false) {
        return error::unknown_error;
    }

    // Set return vnode
    vn = structures::pair<uint64_t, structures::reference_wrapper<vnode>>(
        (*vnode_pair.first).first, _vnodes[(*vnode_pair.first).first]);

    return error::success;
}

influx::vfs::filesystem* influx::vfs::vfs::get_fs_for_file(const influx::vfs::path& file_path) {
    threading::lock_guard lk(_mounts_mutex);

    fs_mount best_fs_match{nullptr, ""};

    // For each mount, check if it's the better match for the file
    for (const auto& mount : _mounts) {
        // First filesystem or better match
        if (best_fs_match.fs == nullptr ||
            (best_fs_match.fs != nullptr && mount.mount_path.is_parent(file_path) &&
             mount.mount_path.amount_of_branches() >
                 best_fs_match.mount_path.amount_of_branches())) {
            best_fs_match = mount;
        }
    }

    return best_fs_match.fs;
}