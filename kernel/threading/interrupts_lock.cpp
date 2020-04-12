#include <kernel/threading/interrupts_lock.h>

#include <kernel/interrupts/interrupt_manager.h>
#include <kernel/kernel.h>

influx::threading::interrupts_lock::interrupts_lock(bool lock) : _locked(lock) {
    // Disable interrupts if the lock need to be locked
    if (lock) {
        kernel::interrupt_manager()->disable_interrupts();
    }
}

influx::threading::interrupts_lock::~interrupts_lock() {
    // If the lock is locked, re-enable interrupts
    if (_locked) {
        kernel::interrupt_manager()->enable_interrupts();
    }
}

void influx::threading::interrupts_lock::lock() {
    // If not locked, disable interrupts
    if (!_locked) {
        kernel::interrupt_manager()->disable_interrupts();
        _locked = true;
    }
}

void influx::threading::interrupts_lock::unlock() {
    // If locked, enable interrupts
    if (_locked) {
        kernel::interrupt_manager()->enable_interrupts();
        _locked = false;
    }
}