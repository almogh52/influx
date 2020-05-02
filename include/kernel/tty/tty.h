#pragma once
#include <kernel/key_event.h>
#include <kernel/structures/string.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/condition_variable.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/mutex.h>

namespace influx {
namespace tty {
class tty_manager;

constexpr char qwerty[128] = {
    0,    27,  '1', '2', '3',  '4', '5', '6', '7',  '8',                /* 9 */
    '9',  '0', '-', '=', '\b',                                          /* Backspace */
    '\t',                                                               /* Tab */
    'q',  'w', 'e', 'r',                                                /* 19 */
    't',  'y', 'u', 'i', 'o',  'p', '[', ']', '\n',                     /* Enter key */
    0,                                                                  /* 29   - Control */
    'a',  's', 'd', 'f', 'g',  'h', 'j', 'k', 'l',  ';',                /* 39 */
    '\'', '`', 0,                                                       /* Left shift */
    '\\', 'z', 'x', 'c', 'v',  'b', 'n',                                /* 49 */
    'm',  ',', '.', '/', 0,                                             /* Right shift */
    '*',  0,                                                            /* Alt */
    ' ',                                                                /* Space bar */
    0,                                                                  /* Caps lock */
    0,                                                                  /* 59 - F1 key ... > */
    0,    0,   0,   0,   0,    0,   0,   0,   0,                        /* < ... F10 */
    0,                                                                  /* 69 - Num lock*/
    0,                                                                  /* Scroll Lock */
    '7',  '8', '9', '-', '4',  '5', '6', '+', '1',  '2', '3', '0', '.', /* Delete Key */
    0,    0,   0,   0,                                                  /* F11 Key */
    0,                                                                  /* F12 Key */
    0, /* All other keys are undefined */
};

constexpr char shifted_qwerty[128] = {
    0,    27,  '!', '@', '#',  '$', '%', '^', '&',  '*',              /* 9 */
    '(',  ')', '_', '+', '\b',                                        /* Backspace */
    '\t',                                                             /* Tab */
    'Q',  'W', 'E', 'R',                                              /* 19 */
    'T',  'Y', 'U', 'I', 'O',  'P', '[', ']', '\n',                   /* Enter key */
    0,                                                                /* 29   - Control */
    'A',  'S', 'D', 'F', 'G',  'H', 'J', 'K', 'L',  ':',              /* 39 */
    '"',  '~', 0,                                                     /* Left shift */
    '|',  'Z', 'X', 'C', 'V',  'B', 'N',                              /* 49 */
    'M',  '<', '>', '?', 0,                                           /* Right shift */
    '*',  0,                                                          /* Alt */
    ' ',                                                              /* Space bar */
    0,                                                                /* Caps lock */
    0,                                                                /* 59 - F1 key ... > */
    0,    0,   0,   0,   0,    0,   0,   0,   0,                      /* < ... F10 */
    0,                                                                /* 69 - Num lock*/
    0,                                                                /* Scroll Lock */
    '7',  '8', '9', '-', '4',  '5', '6', '+', '1',  '2', '3', '0', 0, /* Delete Key */
    0,    0,   0,   0,                                                /* F11 Key */
    0,                                                                /* F12 Key */
    0, /* All other keys are undefined */
};

class tty {
   public:
    tty(uint64_t tty, bool active);

    tty& operator=(const tty& other);

    void activate();
    void deactivate();

    uint64_t stdin_read(char* buf, size_t count);
    void stdout_write(structures::string& str);
    inline void stderr_write(structures::string& str) { stdout_write(str); }

    inline void set_canonical_state(bool canonical) {
        threading::lock_guard lk(_raw_input_mutex);
        _canonical = canonical;
    }
    inline void set_stdin_state(bool enabled) {
        threading::lock_guard lk(_raw_input_mutex);
        _stdin_enabled = enabled;
    }
    inline void set_print_stdin_state(bool print) {
        threading::lock_guard lk(_raw_input_mutex);
        _print_stdin = print;
    }

   private:
    uint64_t _tty;

    bool _active;
    bool _canonical;
    bool _stdin_enabled;
    bool _print_stdin;

    threading::condition_variable _stdin_cv;
    threading::mutex _stdin_mutex;
    threading::mutex _stdout_mutex;

    structures::vector<char> _stdin_buffer;
    structures::vector<structures::string> _stdout_buffer;

    structures::vector<key_event> _raw_input_buffer;
    threading::mutex _raw_input_mutex;
    threading::condition_variable _raw_input_cv;

    void input_thread();
    void print_stdout_buffer();

    friend class tty_manager;
};
};  // namespace tty
};  // namespace influx