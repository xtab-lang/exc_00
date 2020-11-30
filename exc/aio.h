//////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-25
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

#pragma once
#ifndef AIO_H_
#define AIO_H_

namespace exy {
template<typename TWorkItem>
struct WorkProvider {
    using Batch = List<TWorkItem*>;

    const int perBatch;
    SRWLOCK   srw{};

    WorkProvider(int perBatch) : perBatch(perBatch) {}
};

namespace aio {
bool open();

void close();

int ioThreads();

namespace _internal_ {
struct Instance { static auto next(void*) { Assert(0); return false; } };

struct Provider { static auto next(void*) { Assert(0); return false; } };

using FnInstance = bool(Instance::*)(void*);
using FnProvider = bool(Provider::*)(void*);

void post(Instance *instance, FnInstance fnInstance, Provider *provider, FnProvider fnProvider);
} // namespace _internal_

template<typename TInstance, typename TProvider>
void run(TInstance *instance, TProvider &provider) {
    // For each thread, post a work item. All threads will quit if there are
    // no more work items in the {TProvider}.
    _internal_::post((_internal_::Instance*)instance,
                     (_internal_::FnInstance)&TInstance::next,
                     (_internal_::Provider*)&provider, 
                     (_internal_::FnProvider)&TProvider::next);
}
} // namespace aio
} // namespace exy

#endif // AIO_H_