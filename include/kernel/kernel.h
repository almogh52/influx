#pragma once
#include <sys/boot_info.h>

#define OS_NAME "Influx"
#define KERNEL_VERSION "0.2.0"

namespace influx {
	class kernel {
	public:
		static void start(const boot_info info);

	private:
		static void early_kmain(const boot_info info);
		static void kmain(const boot_info info);
	};
};