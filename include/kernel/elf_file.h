#pragma once
#include <kernel/file_segment.h>
#include <kernel/structures/vector.h>
#include <stddef.h>

namespace influx {
class elf_file {
   public:
    elf_file();
    elf_file(size_t fd);

    bool parse();

    inline bool parsed() const { return _parsed; };
    inline const structures::vector<file_segment>& segments() const { return _segments; };
    inline uint64_t entry_address() const { return _entry_address; }

   private:
    bool _parsed;
    size_t _fd;

    uint64_t _entry_address;
    structures::vector<file_segment> _segments;
};
};  // namespace influx