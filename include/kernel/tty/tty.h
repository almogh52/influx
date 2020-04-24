#pragma once
#include <kernel/structures/string.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/condition_variable.h>
#include <kernel/threading/mutex.h>

namespace influx {
namespace tty {
class tty {
   public:
    tty(bool active);

    tty& operator=(const tty& other);

    void activate();
    void deactivate();

    uint64_t stdin_read(char* buf, size_t count);
    void stdout_write(structures::string& str);
    inline void stderr_write(structures::string& str) { stdout_write(str); }

   private:
    bool _active;
    bool _stdin_line_buffered;

    threading::condition_variable _stdin_cv;
    threading::mutex _stdin_mutex;
    threading::mutex _stdout_mutex;

    structures::vector<char> _stdin_buffer;
    structures::vector<structures::string> _stdout_buffer;
};
};  // namespace tty
};  // namespace influx