#pragma once
#include <kernel/structures/string.h>
#include <kernel/structures/vector.h>

namespace influx {
namespace vfs {
class path {
   public:
    path() = default;
    path(const structures::string& path_str);
    inline path(const char* str) : path(structures::string(str)){};

    path& operator=(structures::string path);

    bool is_parent(const path& p) const;

    structures::string string() const;
    inline operator structures::string() const { return string(); }

    inline bool is_relative() const { return !empty() && _branches[0].size() != 0; }
    inline bool is_absolute() const { return !is_relative(); };

    inline bool empty() const { return _branches.empty(); };
    inline bool is_root() const { return _branches.size() == 1 && _branches[0] == ""; };

    inline const structures::vector<structures::string>& branches() const { return _branches; };
    inline size_t amount_of_branches() const { return _branches.size(); }

    inline structures::string base_name() const {
        return _branches.empty() ? "" : *(_branches.end() - 1);
    };
    inline structures::string root_name() const {
        return _branches.size() > 0 ? _branches[0] : "";
    };

    inline structures::string name(size_t i) const { return _branches[i]; }
    inline structures::string operator[](size_t i) const { return name(i); }

    inline bool operator==(const path& p) const { return string() == p.string(); };
    inline bool operator!=(const path& p) const { return !(*this == p); }
    inline bool operator==(structures::string p) const { return string() != p; };
    inline bool operator!=(structures::string p) const { return !(*this == p); }

   private:
    structures::vector<structures::string> _branches;

    static structures::vector<structures::string> build_path(const structures::string& path_str);
};
};  // namespace vfs
};  // namespace influx