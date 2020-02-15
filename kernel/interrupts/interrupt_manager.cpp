#include <descriptor_table_register.h>

#include <kernel/gdt.h>
#include <kernel/interrupts/exception_interrupt_handler.h>
#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/memory/utils.h>
#include <kernel/memory/virtual_allocator.h>

influx::interrupts::interrupt_manager::interrupt_manager()
    : _log("Interrupt Manager"),
      _idt((interrupt_descriptor_t *)memory::virtual_allocator::allocate(IDT_SIZE,
                                                                         PROT_READ | PROT_WRITE)) {
    // Init the IDT
    memory::utils::memset(_idt, 0, IDT_SIZE);
    _log("IDT initialized in the address %p.\n", _idt);

    // Init exception interrupts
    init_exception_interrupts();

    // Load the IDT
    load_idt();
}

void influx::interrupts::interrupt_manager::set_interrupt_service_routine(
    uint8_t interrupt_index, influx::interrupts::interrupt_service_routine isr) {
    interrupt_descriptor_t descriptor{.offset_1 = (uint16_t)(isr.routine_address & 0xFFFF),
                                      .selector = (uint8_t)gdt_selector_types::code_descriptor,
                                      .interrupt_stack_table_index = 0,
                                      .attributes = 0,
                                      .offset_2 = (uint16_t)((isr.routine_address >> 16) & 0xFFFF),
                                      .offset_3 = (uint32_t)(isr.routine_address >> 32),
                                      .zero_2 = 0};

    // Set the attributes of the descriptor
    descriptor.type = (uint16_t)isr.type & 0xF;
    descriptor.present = true;

    // Set the descriptor in the IDT for the ISR
    _idt[interrupt_index] = descriptor;
    _log("ISR (%p) has been set for interrupt %x.\n", isr.routine_address, interrupt_index);
}

void influx::interrupts::interrupt_manager::load_idt() {
    descriptor_table_register_t idtr{.limit = IDT_SIZE, .base_virtual_address = (uint64_t)_idt};

    // Load IDT
    __asm__ __volatile__(
        "lea rax, %0;"
        "lidt [rax];"
        :
        : "m"(idtr)
        : "rax");

    _log("IDT loaded.\n");
}

void influx::interrupts::interrupt_manager::init_exception_interrupts() {
    interrupt_service_routine isr{.type = interrupt_service_routine_type::trap_gate,
                                  .routine_address = (uint64_t)exception_interrupt_handler};

    // Set the exception interrupt handler as the interrupt handler for the first 20 interrupts
    for (uint8_t i = 0; i < 20; i++) {
        set_interrupt_service_routine(i, isr);
    }
}