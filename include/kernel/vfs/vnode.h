#pragma once
#include <kernel/threading/mutex.h>
#include <kernel/vfs/file_info.h>
#include <kernel/vfs/filesystem.h>
#include <stdint.h>

namespace influx {
namespace vfs {
struct vnode {
    inline vnode() : deleted(false), amount_of_open_files(0), fs_data(nullptr) {}
    inline vnode(file_info file_, filesystem *fs_, void *fs_data_)
        : file(file_), fs(fs_), deleted(false), amount_of_open_files(0), fs_data(fs_data_) {}
    inline vnode(const vnode &other)
        : file(other.file),
          fs(other.fs),
          deleted(other.deleted),
          amount_of_open_files(other.amount_of_open_files),
          fs_data(other.fs_data) {}

    inline vnode &operator=(vnode &other) {
        file = other.file;
        fs = other.fs;
        deleted = other.deleted;
        amount_of_open_files = other.amount_of_open_files;
        fs_data = other.fs_data;

        return *this;
    }

    file_info file;
    filesystem *fs;
    bool deleted;
    uint64_t amount_of_open_files;
    void *fs_data;
    threading::mutex file_mutex;
};
};  // namespace vfs
};  // namespace influx