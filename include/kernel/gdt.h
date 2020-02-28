#pragma once

namespace influx {
enum class gdt_selector_types {
    null_descriptor = 0x0,
    code_descriptor = 0x8,
    data_descriptor = 0x10
};
};