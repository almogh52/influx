#include <kernel/vfs/path.h>

influx::vfs::path::path(const influx::structures::string &path_str)
    : _branches(build_path(path_str)) {}

influx::vfs::path::path(influx::structures::vector<influx::structures::string> &branches)
    : _branches(branches) {}

influx::vfs::path &influx::vfs::path::operator=(influx::structures::string path) {
    _branches = build_path(path);
    return *this;
}

bool influx::vfs::path::is_parent(const influx::vfs::path &p) const {
    // The parent should less branches then this path
    if (p._branches.size() >= _branches.size()) {
        return false;
    }

    // For each branch check if it matches the parent's branch
    for (uint64_t i = 0; i < p._branches.size(); i++) {
        if (p._branches[i] != _branches[i]) {
            return false;
        }
    }

    return true;
}

influx::structures::string influx::vfs::path::string() const {
    structures::string path_str;

    // For each branch add it to the path string
    for (const structures::string &branch : _branches) {
        path_str += branch + '/';
    }

    // Remove the last divider
    if (path_str.size() != 1) {
        path_str.resize(path_str.size() - 1);
    }

    return path_str;
}

influx::structures::vector<influx::structures::string> influx::vfs::path::build_path(
    const influx::structures::string &path_str) {
    influx::structures::vector<influx::structures::string> branches;
    uint64_t branch_start = 0, branch_size = 0;

    // For each character check if it's a divider
    for (uint64_t i = 0; i < path_str.size(); i++) {
        // If the character is a divider
        if (path_str[i] == '/') {
            // If this is the first branch or the branch size isn't 0
            if (branch_start == 0 || branch_size > 0) {
                // Add the current branch
                branches += structures::string(path_str.data() + branch_start, branch_size);

                // Check if the relative branch can be trimmed using previous branches
                if (*(branches.end() - 1) == ".." && branches.size() >= 2) {
                    branches.pop_back();
                    branches.pop_back();
                } else if (*(branches.end() - 1) == "." && branches.size() >= 2) {
                    branches.pop_back();
                }
            }

            // Reset branch
            branch_start = i + 1;
            branch_size = 0;
        } else {
            branch_size++;
        }
    }

    // Add the last branch
    if (branch_size != 0) {
        branches += structures::string(path_str.data() + branch_start, branch_size);

        // Check if the relative branch can be trimmed using previous branches
        if (*(branches.end() - 1) == ".." && branches.size() >= 2) {
            branches.pop_back();
            branches.pop_back();
        } else if (*(branches.end() - 1) == "." && branches.size() >= 2) {
            branches.pop_back();
        }
    }

    return branches;
}