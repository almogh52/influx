#pragma once

namespace influx {
namespace vfs {
enum class file_type {
    unknown,
    regular,
    directory,
    character_device,
    block_device,
    fifo,
    socket,
    symbolic_link
};
};
};