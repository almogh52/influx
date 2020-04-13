#pragma once
#include <kernel/logger.h>
#include <kernel/structures/vector.h>
#include <kernel/threading/condition_variable.h>
#include <kernel/threading/executable.h>
#include <kernel/threading/mutex.h>
#include <kernel/threading/thread.h>

#define INIT_PROCESS_PID 1

namespace influx {
namespace threading {
class scheduler;

class init_process {
   private:
    logger _log;

    scheduler *_schd;
    tcb *_task;

    mutex _execs_mutex;
    condition_variable _execs_cv;
    structures::vector<executable> _execs;

    init_process(scheduler *schd);

    void start();
	void queue_exec(executable& exec);

    void main_task();

    friend class scheduler;
};
};  // namespace threading
};  // namespace influx