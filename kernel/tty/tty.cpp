#include <kernel/tty/tty.h>

#include <kernel/algorithm.h>
#include <kernel/console/console.h>
#include <kernel/kernel.h>
#include <kernel/memory/utils.h>
#include <kernel/threading/lock_guard.h>
#include <kernel/threading/unique_lock.h>

influx::tty::tty::tty(bool active) : _active(active), _stdin_line_buffered(true) {}

influx::tty::tty &influx::tty::tty::operator=(const influx::tty::tty &other) {
    _stdin_line_buffered = other._stdin_line_buffered;
    _stdin_buffer = other._stdin_buffer;
    _stdout_buffer = other._stdout_buffer;

    return *this;
}

void influx::tty::tty::activate() {
    // Try to lock both stdin mutex and stdout mutex
    if (kernel::scheduler() != nullptr && kernel::scheduler()->started()) {
        while (true) {
            if (_stdin_mutex.try_lock()) {
                if (_stdout_mutex.try_lock()) {
                    break;
                } else {
                    _stdin_mutex.unlock();
                }
            }
        }
    }

    // Set the tty as active
    _active = true;

    // If there is a console
    if (console::get_console() != nullptr) {
        // Clear the current console
        console::get_console()->clear();

        // Write the stdout history
        for (const auto &str : _stdout_buffer) {
            console::get_console()->print(str);
        }
    }

    // Unlock mutexes
    if (kernel::scheduler() != nullptr && kernel::scheduler()->started()) {
        _stdin_mutex.unlock();
        _stdout_mutex.unlock();
    }
}

void influx::tty::tty::deactivate() {
    // Try to lock both stdin mutex and stdout mutex
    while (true) {
        if (_stdin_mutex.try_lock()) {
            if (_stdout_mutex.try_lock()) {
                break;
            } else {
                _stdin_mutex.unlock();
            }
        }
    }

    // Set the tty as not active
    _active = false;

    // Unlock mutexes
    _stdin_mutex.unlock();
    _stdout_mutex.unlock();
}

uint64_t influx::tty::tty::stdin_read(char *buf, size_t count) {
    threading::unique_lock lk(_stdin_mutex);

    uint64_t amount = 0;
    char *newline_char = nullptr;

    // If line buffered
    if (_stdin_line_buffered) {
        // Search for newline character
        newline_char = algorithm::find(_stdin_buffer.begin(), _stdin_buffer.end(), '\n');

        // If the new line wasn't found
        if (newline_char == _stdin_buffer.end()) {
            _stdin_cv.wait(lk);
        }

        // Try to find again the newline character
        newline_char = algorithm::find(_stdin_buffer.begin(), _stdin_buffer.end(), '\n');
        if (newline_char == _stdin_buffer.end()) {
            return 0;
        }

        // Copy the string
        amount = algorithm::min<uint64_t>(count, newline_char - _stdin_buffer.begin() - 1);
        memory::utils::memcpy(buf, _stdin_buffer.begin(), amount);

        // Resize the vector
        _stdin_buffer = structures::vector<char>(
            _stdin_buffer.begin() + (amount != count ? amount + 1 : amount), _stdin_buffer.end());

        // If there is more in the buffer, notify another
        if (!_stdin_buffer.empty()) {
            _stdin_cv.notify_one();
        }
    } else {
        // If the buffer is empty, wait for new input
        if (_stdin_buffer.empty()) {
            // Wait for new input
            _stdin_cv.wait(lk);
        }

        // Copy the string
        amount = algorithm::min<uint64_t>(count, _stdin_buffer.size());
        memory::utils::memcpy(buf, _stdin_buffer.begin(), amount);

        // Resize the vector
        _stdin_buffer =
            structures::vector<char>(_stdin_buffer.begin() + amount, _stdin_buffer.end());

        // If there is more in the buffer, notify another
        if (!_stdin_buffer.empty()) {
            _stdin_cv.notify_one();
        }
    }

    return amount;
}

void influx::tty::tty::stdout_write(influx::structures::string &str) {
    threading::lock_guard lk(_stdout_mutex);

    // Save the string in the buffer
    _stdout_buffer.push_back(str);

    // Print the string
    if (_active) {
        console::get_console()->print(str);
    }
}