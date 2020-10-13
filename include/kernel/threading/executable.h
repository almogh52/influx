#pragma once
#include <kernel/elf_file.h>
#include <kernel/structures/string.h>
#include <kernel/structures/vector.h>

namespace influx {
namespace threading {
struct executable {
    structures::string name;
    elf_file file;
    structures::vector<structures::string> args;
    structures::vector<structures::string> env;
};
};  // namespace threading
};  // namespace influx