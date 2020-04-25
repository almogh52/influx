#include <kernel/tty/tty_manager.h>

#include <kernel/kernel.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/tty/tty_filesystem.h>

influx::tty::tty_manager::tty_manager() : _active_tty(0) {}

void influx::tty::tty_manager::init() {
    // Create ttys
    for (uint64_t i = 1; i <= AMOUNT_OF_TTYS; i++) {
        _ttys.push_back(tty(false));
    }

    // Activate tty1
    _ttys[0].activate();
}

void influx::tty::tty_manager::reload() {
    // Reload active TTY
    active_tty().deactivate();
    active_tty().activate();
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
    if (tty > AMOUNT_OF_TTYS || tty < 1) {
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