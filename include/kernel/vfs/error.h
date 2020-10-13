#pragma once
#include <stdint.h>

namespace influx {
namespace vfs {
enum error : int64_t {
    success = 0,
    unknown_error = -1,
    io_error = -2,
    file_is_directory = -3,
    file_is_not_directory = -4,
    invalid_file = -5,
    file_not_found = -6,
    vnode_not_found = -7,
    insufficient_permissions = -8,
    invalid_file_access = -9,
    invalid_flags = -10,
    quota_exhausted = -11,
    file_already_exists = -12,
    buffer_too_small = -13,
    invalid_position = -14,
    interrupted = -15,
    is_pipe = -16,
    pipe_closed = -17
};
};
};  // namespace influx