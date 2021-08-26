#pragma once

namespace exy {
namespace aio {
bool open();
void close();

namespace __internal__ {
struct Worker { static auto next(void*) { Assert(0); return false; } };
using WorkerFn = void(Worker::*)(void*);

struct WorkProvider {
    List<BYTE*> &list;
    SRWLOCK      srw{};

    WorkProvider(List<BYTE*> &list) : list(list) {}
    BYTE* pop();
};

void post(Worker *worker, WorkProvider &workProvider, WorkerFn workerFn);
} // namespace aio::__internal__

template<typename TWorker, typename WorkItem>
void run(TWorker &worker, List<WorkItem*> &workList) {
    __internal__::WorkProvider provider{ (List<BYTE*>&)workList };
    __internal__::post((__internal__::Worker*)&worker, 
                       provider, 
                       (__internal__::WorkerFn)&TWorker::run);
}
} // namespace aio
} // namespace exy