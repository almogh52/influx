#pragma once
#include <kernel/vfs/error.h>
#include <memory/protection_flags.h>
#include <stdint.h>

namespace influx {
namespace syscalls {
class utils {
   public:
    static int64_t convert_vfs_error(vfs::error err);
    static bool is_string_in_user_memory(const char *str);
    static bool is_buffer_in_user_memory(const void *buf, uint64_t size,
                                         protection_flags_t permissions);
};
};  // namespace syscalls
};  // namespace influx