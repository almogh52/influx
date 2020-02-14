#include <descriptor_table_register.h>

#include <kernel/gdt.h>
#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/memory/utils.h>
#include <kernel/memory/virtual_allocator.h>

influx::interrupts::interrupt_manager::interrupt_manager()
    : _log("Interrupt Manager"),
      _idt((interrupt_descriptor_t *)memory::virtual_allocator::allocate(IDT_SIZE,
                                                                         PROT_READ | PROT_WRITE)) {
    // Init the IDT
    memory::utils::memset(_idt, 0, IDT_SIZE);
    _log("IDT allocated and initialized in the address %p.\n", _idt);

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
    _log("ISR (address = %p, type = %d) has been set for interrupt %x.", isr.routine_address,
         isr.type, interrupt_index);
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
