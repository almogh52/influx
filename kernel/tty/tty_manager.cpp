#include <kernel/tty/tty_manager.h>

#include <kernel/threading/lock_guard.h>

influx::tty::tty_manager::tty_manager() : _active_tty(0) {}

void influx::tty::tty_manager::init() {
    // Create ttys
    for (uint64_t i = 1; i <= AMOUNT_OF_TTYS; i++) {
        _ttys.push_back(tty(false));
    }

    // Activate tty1
    _ttys[0].activate();
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

void influx::tty::tty_manager::reload() {
    // Reload active TTY
    active_tty().deactivate();
    active_tty().activate();
}