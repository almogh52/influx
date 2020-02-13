#include <descriptor_table_register.h>
#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/memory/virtual_allocator.h>

influx::interrupts::interrupt_manager::interrupt_manager()
    : _log("Interrupt Manager"),
      _idt((interrupt_descriptor_t *)memory::virtual_allocator::allocate(IDT_SIZE,
                                                                         PROT_READ | PROT_WRITE)) {
    _log("IDT allocated in %p.\n", _idt);

	// Load the IDT
	load_idt();
}

void influx::interrupts::interrupt_manager::load_idt() {
    descriptor_table_register_t idtr {.limit = IDT_SIZE, .base_virtual_address = (uint64_t)_idt};

    // Load IDT
    __asm__ __volatile__(
        "lea rax, %0;"
        "lidt [rax];"
        :
        : "m"(idtr)
        : "rax");
		
    _log("IDT loaded.\n");
}