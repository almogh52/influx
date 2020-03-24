#pragma once
#include <kernel/drivers/ata/slice.h>
#include <kernel/logger.h>
#include <kernel/structures/vector.h>
#include <kernel/vfs/dir_entry.h>
#include <kernel/vfs/file.h>
#include <kernel/vfs/path.h>

namespace influx {
namespace vfs {
class filesystem {
   public:
    filesystem(const structures::string& fs_name, const drivers::ata::drive_slice& drive)
        : _log(fs_name + " Filesystem", console_color::pink), _name(fs_name), _drive(drive){};
    virtual ~filesystem(){};
    virtual bool mount() = 0;
    virtual size_t read(void* fs_file_info, char* buffer, size_t count, size_t offset,
                        size_t& amount_read) = 0;
    virtual size_t write(void* fs_file_info, const char* buffer, size_t count, size_t offset,
                         size_t& amount_written) = 0;
    virtual size_t get_file_info(void* fs_file_info, file& file) = 0;
    virtual size_t entries(void* fs_file_info, structures::vector<dir_entry>& entries) = 0;
    virtual size_t create_file(const path& file_path) = 0;
    virtual size_t create_dir(const path& dir_path) = 0;
    virtual size_t remove(void* fs_file_info) = 0;
    virtual void* get_fs_file_info(const path& file_path) = 0;

    const structures::string& name() const { return _name; }

   protected:
    logger _log;

    structures::string _name;
    drivers::ata::drive_slice _drive;
};
};  // namespace vfs
};  // namespace influx