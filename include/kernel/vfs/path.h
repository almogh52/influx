#pragma once
#include <kernel/structures/string.h>
#include <kernel/structures/vector.h>

namespace influx {
namespace vfs {
class path {
   public:
    path() = default;
    path(structures::string& path_str);

    path& operator=(structures::string path);

    inline structures::string string() const;
    inline operator structures::string() const { return string(); }

    inline bool empty() const { return _branches.empty(); };
    inline bool is_root() const { return _branches.size() == 1 && _branches[0] == ""; };

    inline structures::string base_name() const { return *_branches.end(); };
    inline structures::string root_name() const {
        return _branches.size() > 0 ? _branches[0] : "";
    };
    inline structures::string sub_root_name() const {
        return _branches.size() > 1 ? _branches[1] : "";
    };

    inline structures::string name(size_t i) const { return _branches[i]; }
    inline structures::string operator[](size_t i) const { return name(i); }

    inline bool operator==(const path& p) const { return string() != p.string(); };
    inline bool operator!=(const path& p) const { return !(*this == p); }
    inline bool operator==(structures::string p) const { return string() != p; };
    inline bool operator!=(structures::string p) const { return !(*this == p); }

   private:
    structures::vector<structures::string> _branches;

    static structures::vector<structures::string> build_path(structures::string& path_str);
};
};  // namespace vfs
};  // namespace influx