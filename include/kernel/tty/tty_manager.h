#pragma once
#include <kernel/drivers/ps2_keyboard.h>
#include <kernel/key_event.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/condition_variable.h>
#include <kernel/threading/mutex.h>
#include <kernel/tty/tty.h>
#include <stdint.h>

#define AMOUNT_OF_TTYS 15
#define KERNEL_TTY 1

namespace influx {
namespace tty {
class tty_manager {
   public:
    tty_manager();

    void init();
    void reload();
    void start_input_threads();
    void create_tty_vnodes();

    tty& active_tty();
    void set_active_tty(uint64_t tty);
    tty& get_tty(uint64_t tty);

    uint64_t get_tty_vnode(uint64_t tty);
    structures::string get_tty_name(uint64_t vnode_index);

   private:
    uint64_t _active_tty;
    threading::mutex _active_tty_mutex;

    structures::vector<tty> _ttys;
    structures::vector<uint64_t> _ttys_vnodes;

    void raw_input_thread();
};
};  // namespace tty
};  // namespace influx