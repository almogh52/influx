#pragma once
#include <sys/boot_info.h>
#include <kernel/interrupts/interrupt_manager.h>

#define OS_NAME "Influx"
#define KERNEL_VERSION "0.2.0"

namespace influx {
	class kernel {
	public:
		static void start(const boot_info info);

	private:
		inline static interrupts::interrupt_manager *_interrupt_manager = nullptr;

		static void early_kmain(const boot_info info);
		static void kmain(const boot_info info);
	};
};