#pragma once
#include <stdint.h>

namespace influx {
namespace vfs {
enum error : int64_t {
    success,
    io_error = -1,
    file_is_directory = -2,
    file_is_not_directory = -3,
    invalid_file = -4,
    file_not_found = -5,
    vnode_not_found = -6
};
};
};  // namespace influx