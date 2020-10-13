#pragma once
#include <kernel/vfs/filesystem.h>

namespace influx {
namespace vfs {
class pipe_manager;

class pipe_filesystem : public filesystem {
   public:
    pipe_filesystem(pipe_manager* manager);

    inline virtual bool mount(const path& mount_path) { return false; }
    virtual error read(void* fs_file_info, char* buffer, size_t count, size_t offset,
                       size_t& amount_read);
    virtual error write(void* fs_file_info, const char* buffer, size_t count, size_t offset,
                        size_t& amount_written);
    virtual error get_file_info(void* fs_file_info, file_info& file);
    inline virtual error read_dir_entries(void* fs_file_info, size_t offset,
                                          structures::vector<dir_entry>& entries,
                                          size_t dirent_buffer_size, size_t& amount_read) {
        return error::file_is_not_directory;
    }
    inline virtual error create_file(const path& file_path, file_permissions permissions,
                                     void** fs_file_info_ptr) {
        return error::insufficient_permissions;
    }
    inline virtual error create_dir(const path& dir_path, file_permissions permissions,
                                    void** fs_file_info_ptr) {
        return error::insufficient_permissions;
    }
    virtual void duplicate_open_file(const open_file& file, void* fs_file_info);
    virtual void close_open_file(const open_file& file, void* fs_file_info);
    inline virtual error unlink_file(const path& file_path) {
        return error::insufficient_permissions;
    }
    inline virtual void* get_fs_file_data(const path& file_path) { return nullptr; }
    virtual bool compare_fs_file_data(void* fs_file_data_1, void* fs_file_data_2);

   private:
    pipe_manager* _manager;
};
};  // namespace vfs
};  // namespace influx