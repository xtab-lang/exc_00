#include "pch.h"

#define THREADS_PER_CORE 2
#define MIN_THREADS      4

namespace exy {

static auto numberOfThreads() {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    auto n = INT(si.dwNumberOfProcessors);
    auto m = n * THREADS_PER_CORE;
    return max(m, MIN_THREADS);
}

struct Iocp {
    HANDLE        handle{};
    List<HANDLE>  threads{};
    volatile LONG activeThreads{};
    volatile LONG isRunning{};

    auto open() {
        auto errors = 0;
        auto      n = numberOfThreads();
        traceln("Starting %i#<green> threads (%i#<green> per core)", n, THREADS_PER_CORE);
        handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, n);
        if (handle == nullptr) {
            OsError("CreateIoCompletionPort", nullptr);
            return false;
        }
        isRunning = TRUE;
        for (auto i = 0; i < n; ++i) {
            auto thread = CreateThread(nullptr, 0, loop, this, 0, nullptr);
            if (thread == nullptr) {
                OsError("CreateThread", nullptr);
                ++errors;
            } else {
                threads.append(thread);
            }
        }
        waitUntilActiveThreadCountIs(n);
        return errors == 0;
    }

    auto close() {
        if (isRunning == FALSE) {
            return;
        }
        traceln("Stopping %i#<green> threads", threads.length);
        isRunning = FALSE;
        if (WaitForMultipleObjects(threads.length, threads.items, 1, INFINITE) != WAIT_OBJECT_0) {
            OsError("WaitForMultipleObjects", "Expected return value to be '%c#<green>'", "WAIT_OBJECT_0");
        }
        Assert(activeThreads == 0);
        threads.dispose([](HANDLE thread) { 
            if (CloseHandle(thread) == FALSE) {
                OsError("CloseHandle", nullptr);
            }
        });
        if (handle != nullptr) {
            if (CloseHandle(handle) == FALSE) {
                OsError("CloseHandle", nullptr);
            }
            handle = nullptr;
        }
    }

    void waitUntilActiveThreadCountIs(INT n) {
        auto counter = &activeThreads;
        while (*counter != n) {
            if (WaitOnAddress(counter, (void*)counter, sizeof(*counter), 16) == 0) {
                if (GetLastError() != ERROR_TIMEOUT) {
                    OsError("WaitOnAddress", nullptr);
                    break;
                }
            }
        }
    }

    static DWORD loop(void *param) {
        auto iocp = (Iocp*)param;
        InterlockedIncrement(&iocp->activeThreads);
        WakeByAddressSingle((void*)&iocp->activeThreads);

        traceln("   thread(%i#<green>#0x) started", GetCurrentThreadId());

        while (iocp->isRunning == TRUE) {
            DWORD bytesTransferred{};
            ULONG_PTR completionKey{};
            OVERLAPPED *overlapped{};
            auto status = GetQueuedCompletionStatus(iocp->handle, &bytesTransferred,
                                                    &completionKey, &overlapped, 1);
            if (status == FALSE) {
                auto hResult = GetLastError();
                if (hResult == WAIT_TIMEOUT) {
                    continue;
                }
                OsError("GetQueuedCompletionStatus", nullptr);
            } else {
                Assert(0);
                /*auto runner = (aio::_internal_::Runner*)overlapped;
                Assert(runner);
                Assert(counter);
                runner->next(counter);*/
            }
        }

        InterlockedDecrement(&iocp->activeThreads);
        WakeByAddressSingle((void*)&iocp->activeThreads);

        traceln("   thread(%i#<green>#0x) exited", GetCurrentThreadId());

        return 0;
    }
};

static Iocp iocp{};

namespace aio {
bool open() {
    return iocp.open();
}

void close() {
    iocp.close();
}
} // namespace aio
} // namespace exy