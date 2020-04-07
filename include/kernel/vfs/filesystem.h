#pragma once
#include <kernel/drivers/ata/slice.h>
#include <kernel/logger.h>
#include <kernel/structures/vector.h>
#include <kernel/vfs/dir_entry.h>
#include <kernel/vfs/error.h>
#include <kernel/vfs/file_info.h>
#include <kernel/vfs/path.h>

namespace influx {
namespace vfs {
class filesystem {
   public:
    inline filesystem(const structures::string& fs_name, const drivers::ata::drive_slice& drive)
        : _log(fs_name + " Filesystem", console_color::pink), _name(fs_name), _drive(drive){};
    inline virtual ~filesystem(){};
    virtual bool mount(const path& mount_path) = 0;
    virtual error read(void* fs_file_info, char* buffer, size_t count, size_t offset,
                       size_t& amount_read) = 0;
    virtual error write(void* fs_file_info, const char* buffer, size_t count, size_t offset,
                        size_t& amount_written) = 0;
    virtual error get_file_info(void* fs_file_info, file_info& file) = 0;
    virtual error entries(void* fs_file_info, size_t count, size_t offset,
                          structures::vector<dir_entry>& entries) = 0;
    virtual error create_file(const path& file_path, file_permissions permissions,
                              void** fs_file_info_ptr) = 0;
    virtual error create_dir(const path& dir_path, file_permissions permissions,
                             void** fs_file_info_ptr) = 0;
    virtual error unlink_file(const path& file_path) = 0;
    virtual void* get_fs_file_data(const path& file_path) = 0;
    virtual bool compare_fs_file_data(void* fs_file_data_1, void* fs_file_data_2) = 0;

    inline const structures::string& name() const { return _name; }
    inline const drivers::ata::drive_slice& drive() const { return _drive; }

   protected:
    logger _log;

    structures::string _name;
    drivers::ata::drive_slice _drive;
};
};  // namespace vfs
};  // namespace influx