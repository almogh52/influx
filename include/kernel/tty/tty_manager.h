#pragma once
#include <kernel/structures/vector.h>
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
    void create_tty_vnodes();

    tty& active_tty();
    void set_active_tty(uint64_t tty);
    tty& get_tty(uint64_t tty);

    uint64_t get_tty_vnode(uint64_t tty);

   private:
    uint64_t _active_tty;
    threading::mutex _active_tty_mutex;

    structures::vector<tty> _ttys;
    structures::vector<uint64_t> _ttys_vnodes;
};
};  // namespace tty
};  // namespace influx