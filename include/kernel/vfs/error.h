#pragma once

namespace influx {
namespace vfs {
enum class error {
	success,
	io_error,
	file_is_directory,
	file_is_not_directory,
	invalid_file
};
};
};