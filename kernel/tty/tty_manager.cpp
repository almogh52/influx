#include <kernel/tty/tty_manager.h>

#include <kernel/kernel.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/unique_lock.h>
#include <kernel/tty/tty_filesystem.h>
#include <kernel/utils.h>

influx::tty::tty_manager::tty_manager() : _active_tty(KERNEL_TTY - 1) {}

void influx::tty::tty_manager::init() {
    // Create ttys
    for (uint64_t i = 1; i <= AMOUNT_OF_TTYS; i++) {
        _ttys.push_back(tty(i, false));
    }

    // Activate tty1
    _ttys[KERNEL_TTY - 1].set_stdin_state(false);
    _ttys[KERNEL_TTY - 1].activate();
}

void influx::tty::tty_manager::reload() {
    // Reload active TTY
    active_tty().deactivate();
    active_tty().activate();
}

void influx::tty::tty_manager::start_input_threads() {
    // Start the kernel thread for the input of each tty
    for (auto &tty_obj : _ttys) {
        kernel::scheduler()->create_kernel_thread(
            utils::method_function_wrapper<tty, &tty::input_thread>, &tty_obj);
    }

    // Create the kernel thread for the raw input thread
    kernel::scheduler()->create_kernel_thread(
        utils::method_function_wrapper<tty_manager, &tty_manager::raw_input_thread>, this);
}

void influx::tty::tty_manager::create_tty_vnodes() {
    threading::lock_guard lk(kernel::vfs()->_vnodes_mutex);

    vfs::filesystem *fs = new tty_filesystem();

    void *fs_file_info = nullptr;
    vfs::file_info file;

    structures::pair<uint64_t, structures::reference_wrapper<vfs::vnode>> vnode(
        0, kernel::vfs()->_vnodes.empty_item());

    // Create vnode for each TTY
    for (uint64_t tty = 1; tty <= AMOUNT_OF_TTYS; tty++) {
        fs_file_info = new uint64_t(tty);
        fs->get_file_info(fs_file_info, file);
        kernel::vfs()->create_vnode_for_file(fs, fs_file_info, file, vnode);
        _ttys_vnodes.push_back(vnode.first);
    }
}

influx::tty::tty &influx::tty::tty_manager::active_tty() {
    threading::lock_guard lk(_active_tty_mutex);
    return _ttys[_active_tty];
}

void influx::tty::tty_manager::set_active_tty(uint64_t tty) {
    threading::lock_guard lk(_active_tty_mutex);

    // Check valid TTY
    if (tty == _active_tty + 1 || tty > AMOUNT_OF_TTYS || tty < 1) {
        return;
    }

    // Deactivate current TTY
    _ttys[_active_tty].deactivate();

    // Activate new TTY
    _ttys[tty - 1].activate();
    _active_tty = tty - 1;
}

influx::tty::tty &influx::tty::tty_manager::get_tty(uint64_t tty) { return _ttys[tty - 1]; }
uint64_t influx::tty::tty_manager::get_tty_vnode(uint64_t tty) { return _ttys_vnodes[tty - 1]; }

void influx::tty::tty_manager::raw_input_thread() {
    drivers::ps2_keyboard *drv =
        (drivers::ps2_keyboard *)kernel::driver_manager()->get_driver("PS/2 Keyboard");
    key_event key_evt;

    while (true) {
        // Get key event
        key_evt = drv->wait_for_key_event();

        tty &active = active_tty();
        threading::lock_guard lk(active._raw_input_mutex);

        // Push the key event to the input buffer
        active._raw_input_buffer.push_back(key_evt);

        // Notify the input thread
        active._raw_input_cv.notify_one();
    }
}