#include "pch.h"

namespace exy {
namespace aio {
struct Iocp {
    HANDLE        handle{};
    List<HANDLE>  threads{};
    volatile long active{};
    volatile long running{};

    bool open();
    void close();
    static DWORD loop(void*);
    static void waitUntil(volatile long *counter, int n);
    static void waitUntilZero(volatile long *counter);
};

static Iocp iocp{};

bool open() {
    return iocp.open();
}

void close() {
    iocp.close();

}

int ioThreads() {
    return iocp.threads.length;
}

namespace _internal_ {
struct Runner {
    OVERLAPPED  overlapped;
    Instance   *instance;
    FnInstance  fnInstance;
    Provider   *provider;
    FnProvider  fnProvider;
    List<char*> batch;

    Runner(Instance *instance, FnInstance fnInstance, Provider *provider, FnProvider fnProvider)
        : instance(instance), fnInstance(fnInstance), provider(provider), fnProvider(fnProvider) {}

    void next(volatile long *counter) {
        while ((provider->*fnProvider)((void*)&batch)) {
            for (auto i = 0; i < batch.length; i++) {
                (instance->*fnInstance)(batch.items[i]);
            }
            batch.clear();
        }

        batch.dispose();
        MemFree(this);

        InterlockedDecrement(counter);
        WakeByAddressSingle(&counter);
    }
};

void post(Instance *instance, FnInstance fnInstance, Provider *provider, FnProvider fnProvider) {
    // Post a {Runner} on each CPU core and wait for all to return.
    auto runners = iocp.threads.length;
    volatile long counter = runners;
    for (auto i = 0; i < runners; i++) {
        auto runner = MemAlloc<_internal_::Runner>();
        runner = new(runner) _internal_::Runner{ instance, fnInstance, provider, fnProvider };
        PostQueuedCompletionStatus(iocp.handle, 0, (ULONG_PTR)&counter, (OVERLAPPED*)runner);
    }
    Iocp::waitUntilZero(&counter);
}
} // namespace _internal_

static int numberOfThreads() {
    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    if (sysInfo.dwNumberOfProcessors < 2) {
        return 2;
    }
    return sysInfo.dwNumberOfProcessors;
}

bool Iocp::open() {
    auto errors = 0;

    auto n = numberOfThreads();

    traceln("Starting %i#<green> threads", n);

    handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, n);

    if (handle == nullptr) {
        OsError("CreateIoCompletionPort", nullptr);
        return false;
    }

    running = 1;
    for (auto i = 0; i < n; i++) {
        auto threadHandle = CreateThread(nullptr, 0, loop, nullptr, 0, nullptr);
        if (threadHandle != nullptr) {
            threads.append(threadHandle);
        } else {
            OsError("CreateThread", nullptr);
            ++errors;
        }
    }

    waitUntil(&active, n);

    return errors == 0;
}

void Iocp::close() {
    if (running == 0) return;
    // Signal that we are exiting and wait for each thread to exit.
    traceln("Closing %i#<green> threads", threads.length);
    running = 0;
    if (WaitForMultipleObjects(threads.length, threads.items, 1, INFINITE) != WAIT_OBJECT_0) {
        OsError("WaitForMultipleObjects", "Expected return value to be '%c#<green>'", "WAIT_OBJECT_0");
    }

    Assert(active == 0);

    // Dispose each thread handle.
    threads.dispose([](HANDLE threadHandle) { CloseHandle(threadHandle); });

    // Dispose IOCP handle.
    if (handle != nullptr) {
        if (CloseHandle(handle) == 0) {
            OsError("CloseHandle", nullptr);
        }
        handle = nullptr;
    }
}

DWORD Iocp::loop(void*) {
    InterlockedIncrement(&iocp.active);
    WakeByAddressSingle((void*)&iocp.active);

    traceln("   thread(%i#<green>) started", GetCurrentThreadId());

    while (iocp.running != 0) {
        DWORD bytesTransferred{};
        volatile long *counter{};
        OVERLAPPED *overlapped{};
        auto status = GetQueuedCompletionStatus(iocp.handle, &bytesTransferred,
                                                (PULONG_PTR)&counter, &overlapped, 1);
        if (status == 0) {
            auto hResult = GetLastError();
            if (hResult == WAIT_TIMEOUT) {
                continue;
            }
            OsError("GetQueuedCompletionStatus", nullptr);
        } else {
            auto runner = (aio::_internal_::Runner*)overlapped;
            Assert(runner);
            Assert(counter);
            runner->next(counter);
        }
    }

    InterlockedDecrement(&iocp.active);
    WakeByAddressSingle((void*)&iocp.active);

    traceln("   thread(%i#<green>) exited", GetCurrentThreadId());

    return 0;
}

void Iocp::waitUntil(volatile long *counter, int n) {
    while (*counter != n) {
        if (WaitOnAddress(counter, (void*)counter, sizeof(*counter), 16) == 0) {
            if (GetLastError() != ERROR_TIMEOUT) {
                OsError("WaitOnAddress", nullptr);
                break;
            }
        }
    }
}

void Iocp::waitUntilZero(volatile long *counter) {
    waitUntil(counter, 0);
}

} // namespace aio
} // namespace exy