#pragma once
#include <kernel/segment.h>
#include <kernel/structures/vector.h>
#include <stddef.h>

namespace influx {
class elf_file {
   public:
    elf_file(size_t fd);

    bool parse();

    inline const structures::vector<segment>& segments() const { return _segments; };
    inline uint64_t entry_address() const { return _entry_address; }

   private:
    size_t _fd;

    uint64_t _entry_address;
    structures::vector<segment> _segments;
};
};  // namespace influx