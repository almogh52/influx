#pragma once

namespace influx {
namespace vfs {
enum open_flags { read = 0x1, write = 0x2, append = 0x4, directory = 0x8, create = 0x10 };
};
};  // namespace influx