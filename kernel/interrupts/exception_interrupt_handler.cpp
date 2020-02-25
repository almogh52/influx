#include <kernel/interrupts/exception_interrupt_handler.h>

#include <kernel/logger.h>
#include <kernel/panic.h>

__attribute__((interrupt)) void influx::interrupts::exception_interrupt_handler(
    influx::interrupts::interrupt_frame *frame, uint64_t error_code) {
    logger log("Exception Interrupt");
    log("CPU Exception occurred with code %x in address %p.\n", error_code, frame->rip);

	// Panic
	kpanic();
}