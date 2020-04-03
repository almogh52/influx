#pragma once

namespace influx {
namespace vfs {
enum access_type { read = 0x1, write = 0x2, append = 0x4 };
};
};  // namespace influx