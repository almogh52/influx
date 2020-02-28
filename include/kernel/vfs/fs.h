#pragma once
#include <kernel/structures/vector.h>
#include <kernel/vfs/file.h>
#include <kernel/vfs/path.h>

namespace influx {
namespace vfs {
class fs {
   public:
    virtual ~file_system(){};
    virtual size_t read(const path& file_path, char* buffer, size_t count, size_t offset,
                        size_t& amount_read) = 0;
    virtual size_t write(const path& file_path, const char* buffer, size_t count, size_t offset,
                         size_t& amount_written) = 0;
    virtual size_t get_file_info(const path& file_path, file& file) = 0;
    virtual size_t entries(const path& dir_path, structures::vector<file>& contents) = 0;
    virtual size_t create_file(const path& file_path) = 0;
    virtual size_t create_dir(const path& dir_path) = 0;
    virtual size_t remove(const path& file_path) = 0;
};
};  // namespace vfs
};  // namespace influx