#pragma once

namespace influx {
namespace drivers {
enum class ata_command { identify = 0xEC, read = 0x20, write = 0x30 };
};
};  // namespace influx