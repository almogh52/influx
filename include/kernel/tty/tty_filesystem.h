#include <kernel/vfs/filesystem.h>

#include <kernel/structures/vector.h>
#include <kernel/threading/mutex.h>

namespace influx {
namespace tty {
class tty_filesystem : public vfs::filesystem {
   public:
    tty_filesystem();

    inline virtual bool mount(const vfs::path& mount_path) { return false; }
    virtual vfs::error read(void* fs_file_info, char* buffer, size_t count, size_t offset,
                            size_t& amount_read);
    virtual vfs::error write(void* fs_file_info, const char* buffer, size_t count, size_t offset,
                             size_t& amount_written);
    virtual vfs::error get_file_info(void* fs_file_info, vfs::file_info& file);
    inline virtual vfs::error read_dir_entries(void* fs_file_info, size_t offset,
                                               structures::vector<vfs::dir_entry>& entries,
                                               size_t dirent_buffer_size, size_t& amount_read) {
        return vfs::error::file_is_not_directory;
    }
    inline virtual vfs::error create_file(const vfs::path& file_path,
                                          vfs::file_permissions permissions,
                                          void** fs_file_info_ptr) {
        return vfs::error::insufficient_permissions;
    }
    inline virtual vfs::error create_dir(const vfs::path& dir_path,
                                         vfs::file_permissions permissions,
                                         void** fs_file_info_ptr) {
        return vfs::error::insufficient_permissions;
    }
    inline virtual void duplicate_open_file(const vfs::open_file& file, void* fs_file_info){};
    inline virtual void close_open_file(const vfs::open_file& file, void* fs_file_info){};
    inline virtual vfs::error unlink_file(const vfs::path& file_path) {
        return vfs::error::insufficient_permissions;
    }
    inline virtual void* get_fs_file_data(const vfs::path& file_path) { return nullptr; }
    virtual bool compare_fs_file_data(void* fs_file_data_1, void* fs_file_data_2);

   private:
    uint64_t _creation_time;

    structures::vector<uint64_t> _tty_access_times;
    threading::mutex _tty_access_times_mutex;
};
};  // namespace tty
};  // namespace influx