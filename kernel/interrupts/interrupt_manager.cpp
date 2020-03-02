#include <descriptor_table_register.h>

#include <kernel/gdt.h>
#include <kernel/interrupts/exception_interrupt_handler.h>
#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/memory/utils.h>
#include <kernel/memory/virtual_allocator.h>
#include <kernel/drivers/pic.h>
#include <kernel/ports.h>
#include <kernel/assert.h>

#define ADD_IRQ_HANDLER(number) set_interrupt_service_routine(PIC1_INTERRUPTS_OFFSET + number, \
                                  {.type = interrupt_service_routine_type::interrupt_gate, \
                                   .routine_address = (uint64_t)irq_interrupt_handler<number>})

influx::interrupts::interrupt_manager::interrupt_manager()
    : _log("Interrupt Manager", console_color::green),
      _idt((interrupt_descriptor_t *)memory::virtual_allocator::allocate(IDT_SIZE,
                                                                         PROT_READ | PROT_WRITE)) {
    // Init the IDT
    memory::utils::memset(_idt, 0, IDT_SIZE);
    _log("IDT initialized in the address %p.\n", _idt);

    // Register exception interrupts
    _log("Resitering exception interrupt handlers..\n");
    register_exception_interrupts();

    // Register exception interrupts
    _log("Resitering PIC interrupt handlers..\n");
    register_pic_interrupts();

    // Remap PIC interrupts
    _log("Remmaping PICs interrupt offsets..\n");
    remap_pic_interrupts();

    // Load the IDT
    load_idt();

    // Enable interrupts
    _log("Enabling interrupts..\n");
    enable_interrupts();
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

void influx::interrupts::interrupt_manager::set_irq_handler(uint8_t irq, uint64_t irq_handler_address, void *irq_handler_data)
{
    kassert(irq >= 0 && irq < PIC_INTERRUPT_COUNT);
    kassert(_irqs[irq].handler_address == 0);

    // Set IRQ handler
    _irqs[irq].handler_address = irq_handler_address;
    _irqs[irq].handler_data = irq_handler_data;
    _log("IRQ handler (%p) has been set for IRQ %x.\n", irq_handler_address, irq);
}

void influx::interrupts::interrupt_manager::enable_interrupts() const {
    __asm__ __volatile__("sti");
}

void influx::interrupts::interrupt_manager::disable_interrupts() const {
    __asm__ __volatile__("cli");
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

void influx::interrupts::interrupt_manager::remap_pic_interrupts() {
    // Init master and slave PIC in cascade mode
    ports::out<uint8_t>(ICW1_INIT | ICW1_ICW4, PIC1_COMMAND);
    ports::out<uint8_t>(ICW1_INIT | ICW1_ICW4, PIC2_COMMAND);

    // Set master and slave PIC offsets
    ports::out<uint8_t>(PIC1_INTERRUPTS_OFFSET, PIC1_DATA);
    ports::out<uint8_t>(PIC2_INTERRUPTS_OFFSET, PIC2_DATA);

    // Set cascade exits in the 2 PICs
    ports::out<uint8_t>(
        4, PIC1_DATA);  // Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    ports::out<uint8_t>(2, PIC2_DATA);  // Tell Slave PIC its cascade identity (0000 0010)

    // Set 8086 mode for both PICs
    ports::out<uint8_t>(ICW4_8086, PIC1_DATA);
    ports::out<uint8_t>(ICW4_8086, PIC2_DATA);

    // Activate all IRQs in both PICs
    ports::out<uint8_t>(0, PIC1_DATA);
    ports::out<uint8_t>(0, PIC2_DATA);
}

void influx::interrupts::interrupt_manager::register_exception_interrupts() {
    interrupt_service_routine isr{.type = interrupt_service_routine_type::trap_gate,
                                  .routine_address = (uint64_t)exception_interrupt_handler};

    // Set the exception interrupt handler as the interrupt handler for the first 20 interrupts
    for (uint8_t i = 0; i < 20; i++) {
        set_interrupt_service_routine(i, isr);
    }
}

void influx::interrupts::interrupt_manager::register_pic_interrupts() {
    ADD_IRQ_HANDLER(0);
    ADD_IRQ_HANDLER(1);
    ADD_IRQ_HANDLER(2);
    ADD_IRQ_HANDLER(3);
    ADD_IRQ_HANDLER(4);
    ADD_IRQ_HANDLER(5);
    ADD_IRQ_HANDLER(6);
    ADD_IRQ_HANDLER(7);
    ADD_IRQ_HANDLER(8);
    ADD_IRQ_HANDLER(9);
    ADD_IRQ_HANDLER(10);
    ADD_IRQ_HANDLER(11);
    ADD_IRQ_HANDLER(12);
    ADD_IRQ_HANDLER(13);
    ADD_IRQ_HANDLER(14);
    ADD_IRQ_HANDLER(15);
}