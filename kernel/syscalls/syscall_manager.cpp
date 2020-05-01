#include <kernel/syscalls/syscall_manager.h>

#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/kernel.h>
#include <kernel/syscalls/error.h>
#include <kernel/syscalls/handlers.h>

void influx::syscalls::syscall_interrupt_handler(influx::interrupts::regs *context) {
    threading::signal before_signal = kernel::syscall_manager()->get_signal();

    // Handle the syscall and return result in RAX
    context->rax = kernel::syscall_manager()->handle_syscall(
        (syscall)context->rax, context->rdx, context->rdi, context->rsi, context->r10, context);

    // If the syscall interrupted (the signal had changed during execution), save the return code
    if (before_signal != kernel::syscall_manager()->get_signal() && before_signal == SIGINVL) {
        kernel::syscall_manager()->save_return_value(context->rax);
    }
}

influx::syscalls::syscall_manager::syscall_manager()
    : _log("Syscall Manager", console_color::green) {
    // Register the syscall interrupt handler and set the privilege level of the interrupt to ring 3
    _log("Registering interrupt handler for syscall interrupt..\n");
    kernel::interrupt_manager()->set_interrupt_service_routine(SYSCALL_INTERRUPT,
                                                               (uint64_t)syscall_interrupt_handler);
    kernel::interrupt_manager()->set_interrupt_privilege_level(SYSCALL_INTERRUPT, 3);
}

void idk(void *idk2) {
    influx::threading::signal_info sig_info;

    sig_info.sig = SIGSEGV;
    sig_info.value_int = 5;

    influx::logger l("sdsd");
    l("%d\n", *(uint64_t *)idk2);

    influx::kernel::scheduler()->sleep(2000);
    influx::kernel::scheduler()->send_signal(*(uint64_t *)idk2, -1, sig_info);
}

int64_t influx::syscalls::syscall_manager::handle_syscall(influx::syscalls::syscall syscall,
                                                          uint64_t arg1, uint64_t arg2,
                                                          uint64_t arg3, uint64_t arg4,
                                                          influx::interrupts::regs *context) {
    uint64_t tmp = 0;

    if ((uint64_t)syscall == 0xFF) {
        threading::signal_action act;

        act.handler.action_func = (void (*)(int, influx::threading::signal_info *, void *))arg1;
        act.mask = 0;
        act.restorer = (void (*)())arg2;
        act.flags = 0;

        kernel::scheduler()->set_signal_action(SIGSEGV, act);
        kernel::scheduler()->create_kernel_thread(
            idk, new uint64_t(kernel::scheduler()->get_current_process_id()));
    } else if ((uint64_t)syscall == 0xFFFF) {
        kernel::scheduler()->handle_signal_return(context);
        return context->rax;
    }

    // Call the syscall handler for the wanted syscall
    switch (syscall) {
        case syscall::exit:
            kernel::scheduler()->exit((uint8_t)arg1);
            return 0;

        case syscall::close:
            return handlers::close(arg1);

        case syscall::execve:
            return handlers::execve((char *)arg1, (const char **)arg2, (const char **)arg3);

        case syscall::fork:
            return kernel::scheduler()->fork(*context);

        case syscall::fstat:
            return handlers::fstat(arg1, (stat *)arg2);

        case syscall::getpid:
            return kernel::scheduler()->get_current_process_id();

        case syscall::isatty:
            // TODO: Implement
            return 1;

        case syscall::kill:
            // TODO: Implement
            return -EINVAL;

        case syscall::link:
            // TODO: Implement
            return -EMLINK;

        case syscall::lseek:
            return handlers::lseek(arg1, arg2, (uint8_t)arg3);

        case syscall::open:
            return handlers::open((const char *)arg1, (int)arg2, (int)arg3);

        case syscall::read:
            return handlers::read(arg1, (void *)arg2, arg3);

        case syscall::sbrk:
            tmp = kernel::scheduler()->sbrk(arg1);
            return tmp == 0 ? -ENOMEM : tmp;

        case syscall::stat:
            return handlers::stat((const char *)arg1, (stat *)arg2);

        case syscall::times:
            // TODO: Implement
            return -EFAULT;

        case syscall::unlink:
            return handlers::unlink((const char *)arg1);

        case syscall::wait:
            tmp = (int64_t)kernel::scheduler()->wait_for_child(WAIT_FOR_ANY_PROCESS);
            return (int64_t)tmp < 0 ? -ECHILD : (int64_t)tmp;

        case syscall::write:
            return handlers::write(arg1, (const void *)arg2, arg3);

        case syscall::signal:
            // TODO: Implement
            return -EINVAL;

        case syscall::gettimeofday:
            return handlers::gettimeofday((time::timeval *)arg1, (void *)arg2);

        case syscall::gethostname:
            return handlers::gethostname((char *)arg1, arg2);

        default:
            return -EINVAL;
    }
}

influx::threading::signal influx::syscalls::syscall_manager::get_signal() const {
    return kernel::scheduler()->get_current_task()->value().current_sig;
}

void influx::syscalls::syscall_manager::save_return_value(uint64_t value) const {
    kernel::scheduler()->get_current_task()->value().old_interrupt_regs.rax = value;
}