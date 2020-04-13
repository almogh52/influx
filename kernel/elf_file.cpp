#include <elf.h>
#include <kernel/elf_file.h>
#include <kernel/kernel.h>
#include <kernel/memory/utils.h>
#include <kernel/threading/scheduler.h>

influx::elf_file::elf_file() : _parsed(false), _fd(0), _entry_address(0) {}
influx::elf_file::elf_file(size_t fd) : _parsed(false), _fd(fd), _entry_address(0) {}

bool influx::elf_file::parse() {
    Elf64_Ehdr header;

    Elf64_Phdr program_header;
    segment seg;

    // Read the header from the file
    if (kernel::vfs()->read(_fd, &header, sizeof(Elf64_Ehdr)) < 0) {
        return false;
    }

    // Verify ELF (magic number, 64-bit, x86_64 and executable)
    if (memory::utils::memcmp(&header, ELFMAG, SELFMAG) != 0 || header.e_class != ELFCLASS64 ||
        header.e_machine != 0x3E || header.e_type != ET_EXEC || header.e_phnum == 0 ||
        header.e_shnum == 0) {
        return false;
    }

    // Save the entry address
    _entry_address = header.e_entry;

    // Seek the file to the program header table
    if (kernel::vfs()->seek(_fd, header.e_phoff, vfs::seek_type::set) < 0) {
        return false;
    }

    // Read program headers
    for (uint64_t i = 0; i < header.e_phnum; i++) {
        // Read the program header
        if (kernel::vfs()->seek(_fd, header.e_phoff + i * sizeof(Elf64_Phdr), vfs::seek_type::set) <
                0 ||
            kernel::vfs()->read(_fd, &program_header, sizeof(Elf64_Phdr)) < 0) {
            return false;
        }

        // If the program header is a load program header
        if (program_header.p_type == PT_LOAD) {
            // Init segment object
            seg = segment{.virtual_address = program_header.p_vaddr,
                          .data = structures::dynamic_buffer(program_header.p_memsz),
                          .protection = (protection_flags_t)(
                              (program_header.p_flags & PF_R ? PROT_READ : 0) |
                              (program_header.p_flags & PF_W ? PROT_WRITE : 0) |
                              (program_header.p_flags & PF_X ? PROT_EXEC : 0))};

            // Read segment data
            if (program_header.p_filesz > 0 &&
                (kernel::vfs()->seek(_fd, program_header.p_offset, vfs::seek_type::set) < 0 ||
                 kernel::vfs()->read(_fd, seg.data.data(), program_header.p_filesz) < 0)) {
                return false;
            }

            // Insert the segment to the segments vector
            _segments.push_back(seg);
        }
    }

    // Mark the file as parsed
    _parsed = true;

    return true;
}