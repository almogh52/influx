#pragma once

namespace influx {
namespace drivers {
namespace ata {
enum class command { identify = 0xEC, read = 0x20, write = 0x30, cache_flush = 0xE7 };
};
};  // namespace drivers
};  // namespace influx