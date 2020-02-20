#include <kernel/vfs/path.h>

influx::vfs::path::path(influx::structures::string &path_str) : _branches(build_path(path_str)) {}

influx::vfs::path &influx::vfs::path::operator=(influx::structures::string path) {
    _branches = build_path(path);
    return *this;
}

influx::structures::string influx::vfs::path::string() const
{
	structures::string path_str;

	// For each branch add it to the path string
	for (const structures::string &branch : _branches) {
		path_str += branch + '/';
	}

	// Remove the last divider
	path_str.resize(path_str.size() - 1);

	return path_str;
}

influx::structures::vector<influx::structures::string> influx::vfs::path::build_path(
    influx::structures::string &path_str) {
    influx::structures::vector<influx::structures::string> branches;
    uint64_t branch_start = 0, branch_size = 0;

    // For each character check if it's a divider
    for (uint64_t i = 0; i < path_str.size(); i++) {
        // If the character is a divider
        if (path_str[i] == '/') {
            // Add the current branch
            branches += structures::string(path_str.data() + branch_start, branch_size);

            // Reset branch
            branch_start = i + 1;
            branch_size = 0;
        } else {
            branch_size++;
        }
    }

    return branches;
}