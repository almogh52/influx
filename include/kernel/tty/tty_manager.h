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

    tty& active_tty();
    void set_active_tty(uint64_t tty);
    tty& get_tty(uint64_t tty);

    void reload();

   private:
    uint64_t _active_tty;
    threading::mutex _active_tty_mutex;

    structures::vector<tty> _ttys;
};
};  // namespace tty
};  // namespace influx