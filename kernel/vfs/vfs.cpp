#include <kernel/vfs/vfs.h>

#include <dirent.h>
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

int64_t influx::vfs::vfs::open(const influx::vfs::path& file_path, influx::vfs::open_flags flags,
                               influx::vfs::file_permissions permissions) {
    filesystem* fs = get_fs_for_file(file_path);
    void* fs_file_data = nullptr;

    error err = error::success;
    file_info file;

    structures::pair<uint64_t, structures::reference_wrapper<vnode>> vn(0, _vnodes.empty_item());

    // Verify flags
    if ((!(flags & open_flags::read) && !(flags & open_flags::write))) {
        return error::invalid_flags;
    } else if ((flags & open_flags::directory) && (flags & open_flags::write)) {
        return error::file_is_directory;
    }

    // If no filesystem contains this file path
    if (fs == nullptr) {
        return error::file_not_found;
    }

    // If the file wasn't found in the filesystem
    if ((fs_file_data = fs->get_fs_file_data(file_path)) == nullptr) {
        if (flags & open_flags::create &&
            (err = fs->create_file(file_path, permissions, &fs_file_data)) != error::success) {
            return err;
        } else if (!(flags & open_flags::create)) {
            return error::file_not_found;
        }
    }

    // Read file info from filesystem
    if ((err = fs->get_file_info(fs_file_data, file)) != error::success) {
        delete (uint32_t*)fs_file_data;
        return err;
    }

    // Verify file and access permissions
    if (((flags & open_flags::read) && !file.permissions.read) ||
        ((flags & open_flags::write || flags & open_flags::append) && !file.permissions.write)) {
        delete (uint32_t*)fs_file_data;
        return error::insufficient_permissions;
    }

    // Verify file type
    if (file.type == file_type::regular && (flags & open_flags::directory)) {
        delete (uint32_t*)fs_file_data;
        return error::file_not_found;
    } else if (file.type == file_type::directory && (flags & open_flags::write)) {
        delete (uint32_t*)fs_file_data;
        return error::file_is_directory;
    }

    // Try to find the vnode for the file
    threading::unique_lock vnodes_lk(_vnodes_mutex);
    if (((err = get_vnode_for_file(fs, fs_file_data, vn)) == error::vnode_not_found &&
         (err = create_vnode_for_file(fs, fs_file_data, file, vn)) != error::success) ||
        err != error::success) {
        delete (uint32_t*)fs_file_data;
        return err;
    }

    // Lock the file mutex
    threading::lock_guard file_lk(vn.second->file_mutex);

    // Increase the amount of open files
    vn.second->amount_of_open_files++;

    // Create the file descriptor and return it
    return kernel::scheduler()->add_file_descriptor(
        open_file{.vnode_index = vn.first, .position = 0, .flags = flags});
}

int64_t influx::vfs::vfs::read(size_t fd, void* buf, size_t count) {
    open_file file;
    size_t amount_read = 0;

    error err;

    threading::unique_lock vnodes_lk(_vnodes_mutex);

    // Try to get the open file object
    if ((err = get_open_file_for_fd(fd, file)) != error::success) {
        return err;
    }

    // Create a ref of the vnode
    vnode& vn = _vnodes[file.vnode_index];

    // Check if the file is a directory
    if (vn.file.type == file_type::directory) {
        return error::file_is_directory;
    }

    // Check access for the file
    if (!(file.flags & open_flags::read)) {
        return error::invalid_file_access;
    }

    // Lock the file mutex
    vnodes_lk.unlock();
    threading::lock_guard file_lk(vn.file_mutex);

    // Read the file
    if ((err = vn.fs->read(
             vn.fs_data, (char*)buf,
             algorithm::min<size_t>(
                 count, file.position > vn.file.size ? 0 : vn.file.size - file.position),
             file.position, amount_read)) != error::success) {
        return err;
    }

    // Update file position
    file.position += amount_read;
    kernel::scheduler()->update_file_descriptor(fd, file);

    // Update the file object
    if ((err = vn.fs->get_file_info(vn.fs_data, vn.file)) != error::success) {
        return err;
    }

    return amount_read;
}

int64_t influx::vfs::vfs::write(size_t fd, const void* buf, size_t count) {
    open_file file;
    size_t amount_written = 0;

    error err;

    threading::unique_lock vnodes_lk(_vnodes_mutex);

    // Try to get the open file object
    if ((err = get_open_file_for_fd(fd, file)) != error::success) {
        return err;
    }

    // Create a ref of the vnode
    vnode& vn = _vnodes[file.vnode_index];

    // Check if the file is a directory
    if (vn.file.type == file_type::directory) {
        return error::file_is_directory;
    }

    // Check access for the file
    if (!(file.flags & open_flags::write)) {
        return error::invalid_file_access;
    }

    // Lock the file mutex
    vnodes_lk.unlock();
    threading::lock_guard file_lk(vn.file_mutex);

    // Check append access for the file
    if (file.flags & open_flags::append) {
        // Set position at the end of the file
        file.position = vn.file.size;
    }

    // Write to the file
    if ((err = vn.fs->write(vn.fs_data, (const char*)buf, count, file.position, amount_written)) !=
        error::success) {
        return err;
    }

    // Update file position
    file.position += amount_written;
    kernel::scheduler()->update_file_descriptor(fd, file);

    // Update the file object
    if ((err = vn.fs->get_file_info(vn.fs_data, vn.file)) != error::success) {
        return err;
    }

    return amount_written;
}

int64_t influx::vfs::vfs::get_dir_entries(
    size_t fd, influx::structures::vector<influx::vfs::dir_entry>& entries,
    uint64_t dirent_buffer_size) {
    open_file file;
    size_t amount_read = 0;

    error err;

    threading::unique_lock vnodes_lk(_vnodes_mutex);

    // Try to get the open file object
    if ((err = get_open_file_for_fd(fd, file)) != error::success) {
        return err;
    }

    // Create a ref of the vnode
    vnode& vn = _vnodes[file.vnode_index];

    // Check if the file is a directory
    if (vn.file.type != file_type::directory) {
        return error::file_is_not_directory;
    }

    // Check access for the file
    if (!(file.flags & open_flags::read)) {
        return error::invalid_file_access;
    }

    // Lock the file mutex
    vnodes_lk.unlock();
    threading::lock_guard file_lk(vn.file_mutex);

    // Read the directory's entries
    if ((err = vn.fs->read_dir_entries(vn.fs_data, file.position, entries, dirent_buffer_size,
                                       amount_read)) != error::success) {
        return err;
    }

    // Update file position
    file.position += amount_read;
    kernel::scheduler()->update_file_descriptor(fd, file);

    // Update the file object
    if ((err = vn.fs->get_file_info(vn.fs_data, vn.file)) != error::success) {
        return err;
    }

    return amount_read;
}

uint64_t influx::vfs::vfs::dirent_size_for_dir_entry(influx::vfs::dir_entry& entry) {
    return sizeof(dirent) + entry.name.size() + 1;
}

influx::vfs::error influx::vfs::vfs::get_open_file_for_fd(int64_t fd,
                                                          influx::vfs::open_file& file) {
    return kernel::scheduler()->get_file_descriptor(fd, file);
}

influx::vfs::error influx::vfs::vfs::get_vnode_for_file(
    influx::vfs::filesystem* fs, void* fs_file_data,
    influx::structures::pair<uint64_t, influx::structures::reference_wrapper<influx::vfs::vnode>>&
        vn) {
    // vnodes mutex must be locked here

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
    // vnodes mutex must be locked here

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