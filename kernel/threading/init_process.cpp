#include <kernel/threading/init_process.h>

#include <kernel/elf_file.h>
#include <kernel/threading/scheduler.h>
#include <kernel/threading/unique_lock.h>
#include <kernel/utils.h>

influx::threading::init_process::init_process(influx::threading::scheduler *schd)
    : _log("Init", console_color::red), _schd(schd), _task(nullptr) {}

void influx::threading::init_process::start() {
    kassert(_task == nullptr);

    // Create init main task
    _log("Creating main task..\n");
    _task = _schd->create_kernel_thread(
        utils::method_function_wrapper<init_process, &init_process::main_task>, this, false,
        INIT_PROCESS_PID);

    _log("Init process started.\n");
}

void influx::threading::init_process::queue_exec(influx::threading::executable &exec) {
    kassert(exec.file.parsed());

    unique_lock execs_lk(_execs_mutex);

    // Add the new executable to the list
    _execs.push_back(exec);

    // Notify the main task
    execs_lk.unlock();
    _execs_cv.notify_one();
}

void influx::threading::init_process::main_task() {
    unique_lock execs_lk(_execs_mutex);

    while (true) {
        // Wait for new executable to start
        while (_execs.empty()) {
            _execs_cv.wait(execs_lk);
        }

        // Execute all queued processes
        for (auto &exec : _execs) {
            _log("Starting process '%s'..\n", exec.name.c_str());
            _schd->start_process(exec);
        }

        // Clear execute queue
        _execs.clear();
    }
}