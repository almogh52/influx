#include <descriptor_table_register.h>

#include <kernel/assert.h>
#include <kernel/drivers/pic.h>
#include <kernel/gdt.h>
#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/interrupts/isrs.h>
#include <kernel/kernel.h>
#include <kernel/memory/utils.h>
#include <kernel/memory/virtual_allocator.h>
#include <kernel/ports.h>

#define SET_ISR(n, t) set_isr(n, (uint64_t)isr_##n, t)

#define ADD_IRQ_HANDLER(number) \
    set_interrupt_service_routine(PIC1_INTERRUPTS_OFFSET + number, (uint64_t)irq_interrupt_handler)

uint64_t isrs[AMOUNT_OF_INTERRUPT_DESCRIPTORS];

void influx::interrupts::isr_handler(influx::interrupts::regs *context) {
    // If the ISR isn't null, call it
    if (isrs[context->isr_number] != 0) {
        ((void (*)(regs *))isrs[context->isr_number])(context);
    }
}

void influx::interrupts::exception_interrupt_handler(influx::interrupts::regs *context) {
    // Disable interrupts
    __asm__ __volatile__("cli");

    logger log("Exception Interrupt");
    log("CPU Exception (%x) occurred with code %x in address %p.\n", context->isr_number,
        context->error_code, context->rip);

    // Panic
    kpanic();
}

void influx::interrupts::irq_interrupt_handler(influx::interrupts::regs *context) {
    uint8_t irq_number = (uint8_t)(context->isr_number - PIC1_INTERRUPTS_OFFSET);

    // If the IRQ is on the slave PIC, send EOI to it
    if (irq_number >= 8) {
        influx::ports::out<uint8_t>(PIC_EOI, PIC2_COMMAND);
    }

    // Send EOI to the master PIC
    influx::ports::out<uint8_t>(PIC_EOI, PIC1_COMMAND);

    // Check if there is a handler for this IRQ
    if (kernel::interrupt_manager()->_irqs[irq_number].handler_address != 0) {
        ((void (*)(regs *, void *))kernel::interrupt_manager()->_irqs[irq_number].handler_address)(
            context, influx::kernel::interrupt_manager()->_irqs[irq_number].handler_data);
    }
}

influx::interrupts::interrupt_manager::interrupt_manager()
    : _log("Interrupt Manager", console_color::green),
      _idt((interrupt_descriptor_t *)memory::virtual_allocator::allocate(IDT_SIZE,
                                                                         PROT_READ | PROT_WRITE)) {
    kassert(_idt != nullptr);

    // Init the ISR array
    memory::utils::memset(isrs, 0, sizeof(uint64_t) * AMOUNT_OF_INTERRUPT_DESCRIPTORS);

    // Init the IDT
    memory::utils::memset(_idt, 0, IDT_SIZE);
    _log("IDT initialized in the address %p.\n", _idt);

    _log("Setting ISRs..");
    init_isrs();

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

void influx::interrupts::interrupt_manager::set_interrupt_service_routine(uint8_t interrupt_index,
                                                                          uint64_t isr) {
    kassert(isr != 0);

    // Set the ISR handler
    isrs[interrupt_index] = isr;
    _log("ISR (%p) has been set for interrupt %x.\n", isr, interrupt_index);
}

void influx::interrupts::interrupt_manager::set_interrupt_privilege_level(uint8_t interrupt_index,
                                                                          uint8_t privilege_level) {
    _idt[interrupt_index].privilege_level = privilege_level & 0b11;
    _log("Privilege level of %d has been set for interrupt %x.\n", privilege_level,
         interrupt_index);
}

void influx::interrupts::interrupt_manager::set_irq_handler(uint8_t irq,
                                                            uint64_t irq_handler_address,
                                                            void *irq_handler_data) {
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

void influx::interrupts::interrupt_manager::set_isr(uint8_t interrupt_index, uint64_t isr,
                                                    interrupt_service_routine_type type) {
    interrupt_descriptor_t descriptor{.offset_1 = (uint16_t)(isr & 0xFFFF),
                                      .selector = (uint8_t)gdt_selector_types::code_descriptor,
                                      .interrupt_stack_table_index = 0,
                                      .attributes = 0,
                                      .offset_2 = (uint16_t)((isr >> 16) & 0xFFFF),
                                      .offset_3 = (uint32_t)(isr >> 32),
                                      .zero_2 = 0};

    // Set the attributes of the descriptor
    descriptor.type = (uint8_t)type & 0xF;
    descriptor.present = true;

    // Set the descriptor in the IDT for the ISR
    _idt[interrupt_index] = descriptor;
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
    // Set the exception interrupt handler as the interrupt handler for the first 20 interrupts
    for (uint8_t i = 0; i < 20; i++) {
        set_interrupt_service_routine(i, (uint64_t)exception_interrupt_handler);
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

void influx::interrupts::interrupt_manager::init_isrs() {
    SET_ISR(0, interrupt_service_routine_type::trap_gate);
    SET_ISR(1, interrupt_service_routine_type::trap_gate);
    SET_ISR(2, interrupt_service_routine_type::trap_gate);
    SET_ISR(3, interrupt_service_routine_type::trap_gate);
    SET_ISR(4, interrupt_service_routine_type::trap_gate);
    SET_ISR(5, interrupt_service_routine_type::trap_gate);
    SET_ISR(6, interrupt_service_routine_type::trap_gate);
    SET_ISR(7, interrupt_service_routine_type::trap_gate);
    SET_ISR(8, interrupt_service_routine_type::trap_gate);
    SET_ISR(9, interrupt_service_routine_type::trap_gate);
    SET_ISR(10, interrupt_service_routine_type::trap_gate);
    SET_ISR(11, interrupt_service_routine_type::trap_gate);
    SET_ISR(12, interrupt_service_routine_type::trap_gate);
    SET_ISR(13, interrupt_service_routine_type::trap_gate);
    SET_ISR(14, interrupt_service_routine_type::trap_gate);
    SET_ISR(15, interrupt_service_routine_type::trap_gate);
    SET_ISR(16, interrupt_service_routine_type::trap_gate);
    SET_ISR(17, interrupt_service_routine_type::trap_gate);
    SET_ISR(18, interrupt_service_routine_type::trap_gate);
    SET_ISR(19, interrupt_service_routine_type::trap_gate);
    SET_ISR(20, interrupt_service_routine_type::trap_gate);
    SET_ISR(21, interrupt_service_routine_type::trap_gate);
    SET_ISR(22, interrupt_service_routine_type::trap_gate);
    SET_ISR(23, interrupt_service_routine_type::trap_gate);
    SET_ISR(24, interrupt_service_routine_type::trap_gate);
    SET_ISR(25, interrupt_service_routine_type::trap_gate);
    SET_ISR(26, interrupt_service_routine_type::trap_gate);
    SET_ISR(27, interrupt_service_routine_type::trap_gate);
    SET_ISR(28, interrupt_service_routine_type::trap_gate);
    SET_ISR(29, interrupt_service_routine_type::trap_gate);
    SET_ISR(30, interrupt_service_routine_type::trap_gate);
    SET_ISR(31, interrupt_service_routine_type::trap_gate);
    SET_ISR(32, interrupt_service_routine_type::trap_gate);
    SET_ISR(33, interrupt_service_routine_type::trap_gate);
    SET_ISR(34, interrupt_service_routine_type::trap_gate);
    SET_ISR(35, interrupt_service_routine_type::trap_gate);
    SET_ISR(36, interrupt_service_routine_type::trap_gate);
    SET_ISR(37, interrupt_service_routine_type::trap_gate);
    SET_ISR(38, interrupt_service_routine_type::trap_gate);
    SET_ISR(39, interrupt_service_routine_type::trap_gate);
    SET_ISR(40, interrupt_service_routine_type::trap_gate);
    SET_ISR(41, interrupt_service_routine_type::trap_gate);
    SET_ISR(42, interrupt_service_routine_type::trap_gate);
    SET_ISR(43, interrupt_service_routine_type::trap_gate);
    SET_ISR(44, interrupt_service_routine_type::trap_gate);
    SET_ISR(45, interrupt_service_routine_type::trap_gate);
    SET_ISR(46, interrupt_service_routine_type::trap_gate);
    SET_ISR(47, interrupt_service_routine_type::trap_gate);
    SET_ISR(48, interrupt_service_routine_type::trap_gate);
    SET_ISR(49, interrupt_service_routine_type::trap_gate);
    SET_ISR(50, interrupt_service_routine_type::trap_gate);
    SET_ISR(51, interrupt_service_routine_type::trap_gate);
    SET_ISR(52, interrupt_service_routine_type::trap_gate);
    SET_ISR(53, interrupt_service_routine_type::trap_gate);
    SET_ISR(54, interrupt_service_routine_type::trap_gate);
    SET_ISR(55, interrupt_service_routine_type::trap_gate);
    SET_ISR(56, interrupt_service_routine_type::trap_gate);
    SET_ISR(57, interrupt_service_routine_type::trap_gate);
    SET_ISR(58, interrupt_service_routine_type::trap_gate);
    SET_ISR(59, interrupt_service_routine_type::trap_gate);
    SET_ISR(60, interrupt_service_routine_type::trap_gate);
    SET_ISR(61, interrupt_service_routine_type::trap_gate);
    SET_ISR(62, interrupt_service_routine_type::trap_gate);
    SET_ISR(63, interrupt_service_routine_type::trap_gate);
    SET_ISR(64, interrupt_service_routine_type::trap_gate);
    SET_ISR(65, interrupt_service_routine_type::trap_gate);
    SET_ISR(66, interrupt_service_routine_type::trap_gate);
    SET_ISR(67, interrupt_service_routine_type::trap_gate);
    SET_ISR(68, interrupt_service_routine_type::trap_gate);
    SET_ISR(69, interrupt_service_routine_type::trap_gate);
    SET_ISR(70, interrupt_service_routine_type::trap_gate);
    SET_ISR(71, interrupt_service_routine_type::trap_gate);
    SET_ISR(72, interrupt_service_routine_type::trap_gate);
    SET_ISR(73, interrupt_service_routine_type::trap_gate);
    SET_ISR(74, interrupt_service_routine_type::trap_gate);
    SET_ISR(75, interrupt_service_routine_type::trap_gate);
    SET_ISR(76, interrupt_service_routine_type::trap_gate);
    SET_ISR(77, interrupt_service_routine_type::trap_gate);
    SET_ISR(78, interrupt_service_routine_type::trap_gate);
    SET_ISR(79, interrupt_service_routine_type::trap_gate);
    SET_ISR(80, interrupt_service_routine_type::trap_gate);
    SET_ISR(81, interrupt_service_routine_type::trap_gate);
    SET_ISR(82, interrupt_service_routine_type::trap_gate);
    SET_ISR(83, interrupt_service_routine_type::trap_gate);
    SET_ISR(84, interrupt_service_routine_type::trap_gate);
    SET_ISR(85, interrupt_service_routine_type::trap_gate);
    SET_ISR(86, interrupt_service_routine_type::trap_gate);
    SET_ISR(87, interrupt_service_routine_type::trap_gate);
    SET_ISR(88, interrupt_service_routine_type::trap_gate);
    SET_ISR(89, interrupt_service_routine_type::trap_gate);
    SET_ISR(90, interrupt_service_routine_type::trap_gate);
    SET_ISR(91, interrupt_service_routine_type::trap_gate);
    SET_ISR(92, interrupt_service_routine_type::trap_gate);
    SET_ISR(93, interrupt_service_routine_type::trap_gate);
    SET_ISR(94, interrupt_service_routine_type::trap_gate);
    SET_ISR(95, interrupt_service_routine_type::trap_gate);
    SET_ISR(96, interrupt_service_routine_type::trap_gate);
    SET_ISR(97, interrupt_service_routine_type::trap_gate);
    SET_ISR(98, interrupt_service_routine_type::trap_gate);
    SET_ISR(99, interrupt_service_routine_type::trap_gate);
    SET_ISR(100, interrupt_service_routine_type::trap_gate);
    SET_ISR(101, interrupt_service_routine_type::trap_gate);
    SET_ISR(102, interrupt_service_routine_type::trap_gate);
    SET_ISR(103, interrupt_service_routine_type::trap_gate);
    SET_ISR(104, interrupt_service_routine_type::trap_gate);
    SET_ISR(105, interrupt_service_routine_type::trap_gate);
    SET_ISR(106, interrupt_service_routine_type::trap_gate);
    SET_ISR(107, interrupt_service_routine_type::trap_gate);
    SET_ISR(108, interrupt_service_routine_type::trap_gate);
    SET_ISR(109, interrupt_service_routine_type::trap_gate);
    SET_ISR(110, interrupt_service_routine_type::trap_gate);
    SET_ISR(111, interrupt_service_routine_type::trap_gate);
    SET_ISR(112, interrupt_service_routine_type::trap_gate);
    SET_ISR(113, interrupt_service_routine_type::trap_gate);
    SET_ISR(114, interrupt_service_routine_type::trap_gate);
    SET_ISR(115, interrupt_service_routine_type::trap_gate);
    SET_ISR(116, interrupt_service_routine_type::trap_gate);
    SET_ISR(117, interrupt_service_routine_type::trap_gate);
    SET_ISR(118, interrupt_service_routine_type::trap_gate);
    SET_ISR(119, interrupt_service_routine_type::trap_gate);
    SET_ISR(120, interrupt_service_routine_type::trap_gate);
    SET_ISR(121, interrupt_service_routine_type::trap_gate);
    SET_ISR(122, interrupt_service_routine_type::trap_gate);
    SET_ISR(123, interrupt_service_routine_type::trap_gate);
    SET_ISR(124, interrupt_service_routine_type::trap_gate);
    SET_ISR(125, interrupt_service_routine_type::trap_gate);
    SET_ISR(126, interrupt_service_routine_type::trap_gate);
    SET_ISR(127, interrupt_service_routine_type::trap_gate);
    SET_ISR(128, interrupt_service_routine_type::trap_gate);
    SET_ISR(129, interrupt_service_routine_type::trap_gate);
    SET_ISR(130, interrupt_service_routine_type::trap_gate);
    SET_ISR(131, interrupt_service_routine_type::trap_gate);
    SET_ISR(132, interrupt_service_routine_type::trap_gate);
    SET_ISR(133, interrupt_service_routine_type::trap_gate);
    SET_ISR(134, interrupt_service_routine_type::trap_gate);
    SET_ISR(135, interrupt_service_routine_type::trap_gate);
    SET_ISR(136, interrupt_service_routine_type::trap_gate);
    SET_ISR(137, interrupt_service_routine_type::trap_gate);
    SET_ISR(138, interrupt_service_routine_type::trap_gate);
    SET_ISR(139, interrupt_service_routine_type::trap_gate);
    SET_ISR(140, interrupt_service_routine_type::trap_gate);
    SET_ISR(141, interrupt_service_routine_type::trap_gate);
    SET_ISR(142, interrupt_service_routine_type::trap_gate);
    SET_ISR(143, interrupt_service_routine_type::trap_gate);
    SET_ISR(144, interrupt_service_routine_type::trap_gate);
    SET_ISR(145, interrupt_service_routine_type::trap_gate);
    SET_ISR(146, interrupt_service_routine_type::trap_gate);
    SET_ISR(147, interrupt_service_routine_type::trap_gate);
    SET_ISR(148, interrupt_service_routine_type::trap_gate);
    SET_ISR(149, interrupt_service_routine_type::trap_gate);
    SET_ISR(150, interrupt_service_routine_type::trap_gate);
    SET_ISR(151, interrupt_service_routine_type::trap_gate);
    SET_ISR(152, interrupt_service_routine_type::trap_gate);
    SET_ISR(153, interrupt_service_routine_type::trap_gate);
    SET_ISR(154, interrupt_service_routine_type::trap_gate);
    SET_ISR(155, interrupt_service_routine_type::trap_gate);
    SET_ISR(156, interrupt_service_routine_type::trap_gate);
    SET_ISR(157, interrupt_service_routine_type::trap_gate);
    SET_ISR(158, interrupt_service_routine_type::trap_gate);
    SET_ISR(159, interrupt_service_routine_type::trap_gate);
    SET_ISR(160, interrupt_service_routine_type::trap_gate);
    SET_ISR(161, interrupt_service_routine_type::trap_gate);
    SET_ISR(162, interrupt_service_routine_type::trap_gate);
    SET_ISR(163, interrupt_service_routine_type::trap_gate);
    SET_ISR(164, interrupt_service_routine_type::trap_gate);
    SET_ISR(165, interrupt_service_routine_type::trap_gate);
    SET_ISR(166, interrupt_service_routine_type::trap_gate);
    SET_ISR(167, interrupt_service_routine_type::trap_gate);
    SET_ISR(168, interrupt_service_routine_type::trap_gate);
    SET_ISR(169, interrupt_service_routine_type::trap_gate);
    SET_ISR(170, interrupt_service_routine_type::trap_gate);
    SET_ISR(171, interrupt_service_routine_type::trap_gate);
    SET_ISR(172, interrupt_service_routine_type::trap_gate);
    SET_ISR(173, interrupt_service_routine_type::trap_gate);
    SET_ISR(174, interrupt_service_routine_type::trap_gate);
    SET_ISR(175, interrupt_service_routine_type::trap_gate);
    SET_ISR(176, interrupt_service_routine_type::trap_gate);
    SET_ISR(177, interrupt_service_routine_type::trap_gate);
    SET_ISR(178, interrupt_service_routine_type::trap_gate);
    SET_ISR(179, interrupt_service_routine_type::trap_gate);
    SET_ISR(180, interrupt_service_routine_type::trap_gate);
    SET_ISR(181, interrupt_service_routine_type::trap_gate);
    SET_ISR(182, interrupt_service_routine_type::trap_gate);
    SET_ISR(183, interrupt_service_routine_type::trap_gate);
    SET_ISR(184, interrupt_service_routine_type::trap_gate);
    SET_ISR(185, interrupt_service_routine_type::trap_gate);
    SET_ISR(186, interrupt_service_routine_type::trap_gate);
    SET_ISR(187, interrupt_service_routine_type::trap_gate);
    SET_ISR(188, interrupt_service_routine_type::trap_gate);
    SET_ISR(189, interrupt_service_routine_type::trap_gate);
    SET_ISR(190, interrupt_service_routine_type::trap_gate);
    SET_ISR(191, interrupt_service_routine_type::trap_gate);
    SET_ISR(192, interrupt_service_routine_type::trap_gate);
    SET_ISR(193, interrupt_service_routine_type::trap_gate);
    SET_ISR(194, interrupt_service_routine_type::trap_gate);
    SET_ISR(195, interrupt_service_routine_type::trap_gate);
    SET_ISR(196, interrupt_service_routine_type::trap_gate);
    SET_ISR(197, interrupt_service_routine_type::trap_gate);
    SET_ISR(198, interrupt_service_routine_type::trap_gate);
    SET_ISR(199, interrupt_service_routine_type::trap_gate);
    SET_ISR(200, interrupt_service_routine_type::trap_gate);
    SET_ISR(201, interrupt_service_routine_type::trap_gate);
    SET_ISR(202, interrupt_service_routine_type::trap_gate);
    SET_ISR(203, interrupt_service_routine_type::trap_gate);
    SET_ISR(204, interrupt_service_routine_type::trap_gate);
    SET_ISR(205, interrupt_service_routine_type::trap_gate);
    SET_ISR(206, interrupt_service_routine_type::trap_gate);
    SET_ISR(207, interrupt_service_routine_type::trap_gate);
    SET_ISR(208, interrupt_service_routine_type::trap_gate);
    SET_ISR(209, interrupt_service_routine_type::trap_gate);
    SET_ISR(210, interrupt_service_routine_type::trap_gate);
    SET_ISR(211, interrupt_service_routine_type::trap_gate);
    SET_ISR(212, interrupt_service_routine_type::trap_gate);
    SET_ISR(213, interrupt_service_routine_type::trap_gate);
    SET_ISR(214, interrupt_service_routine_type::trap_gate);
    SET_ISR(215, interrupt_service_routine_type::trap_gate);
    SET_ISR(216, interrupt_service_routine_type::trap_gate);
    SET_ISR(217, interrupt_service_routine_type::trap_gate);
    SET_ISR(218, interrupt_service_routine_type::trap_gate);
    SET_ISR(219, interrupt_service_routine_type::trap_gate);
    SET_ISR(220, interrupt_service_routine_type::trap_gate);
    SET_ISR(221, interrupt_service_routine_type::trap_gate);
    SET_ISR(222, interrupt_service_routine_type::trap_gate);
    SET_ISR(223, interrupt_service_routine_type::trap_gate);
    SET_ISR(224, interrupt_service_routine_type::trap_gate);
    SET_ISR(225, interrupt_service_routine_type::trap_gate);
    SET_ISR(226, interrupt_service_routine_type::trap_gate);
    SET_ISR(227, interrupt_service_routine_type::trap_gate);
    SET_ISR(228, interrupt_service_routine_type::trap_gate);
    SET_ISR(229, interrupt_service_routine_type::trap_gate);
    SET_ISR(230, interrupt_service_routine_type::trap_gate);
    SET_ISR(231, interrupt_service_routine_type::trap_gate);
    SET_ISR(232, interrupt_service_routine_type::trap_gate);
    SET_ISR(233, interrupt_service_routine_type::trap_gate);
    SET_ISR(234, interrupt_service_routine_type::trap_gate);
    SET_ISR(235, interrupt_service_routine_type::trap_gate);
    SET_ISR(236, interrupt_service_routine_type::trap_gate);
    SET_ISR(237, interrupt_service_routine_type::trap_gate);
    SET_ISR(238, interrupt_service_routine_type::trap_gate);
    SET_ISR(239, interrupt_service_routine_type::trap_gate);
    SET_ISR(240, interrupt_service_routine_type::trap_gate);
    SET_ISR(241, interrupt_service_routine_type::trap_gate);
    SET_ISR(242, interrupt_service_routine_type::trap_gate);
    SET_ISR(243, interrupt_service_routine_type::trap_gate);
    SET_ISR(244, interrupt_service_routine_type::trap_gate);
    SET_ISR(245, interrupt_service_routine_type::trap_gate);
    SET_ISR(246, interrupt_service_routine_type::trap_gate);
    SET_ISR(247, interrupt_service_routine_type::trap_gate);
    SET_ISR(248, interrupt_service_routine_type::trap_gate);
    SET_ISR(249, interrupt_service_routine_type::trap_gate);
    SET_ISR(250, interrupt_service_routine_type::trap_gate);
    SET_ISR(251, interrupt_service_routine_type::trap_gate);
    SET_ISR(252, interrupt_service_routine_type::trap_gate);
    SET_ISR(253, interrupt_service_routine_type::trap_gate);
    SET_ISR(254, interrupt_service_routine_type::trap_gate);
    SET_ISR(255, interrupt_service_routine_type::trap_gate);
}