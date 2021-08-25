#pragma once

// 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed. Specify /EHsc
#pragma warning(disable: 4577)

// unreferenced inline function has been removed
#pragma warning(disable: 4514)

// assignment within conditional expression
#pragma warning(disable: 4706)

#define _AMD64_
#undef _INC_WINDOWS
#include <winapifamily.h>
#include <intrin.h>
#include <sdkddkver.h>
#include <windef.h>
#include <WinBase.h>
#include <stdlib.h>
#include <new>

namespace exy {
namespace meta {
template<typename T> struct RemoveReference;
template<typename T> struct RemoveReference { using Type = T; };
template<typename T> struct RemoveReference<T&> { using Type = T; };
template<typename T> struct RemoveReference<T&&> { using Type = T; };
template<typename T> T&& move(T&& t) {
    using RValueReference = typename RemoveReference<T>::Type&&;
    return static_cast<RValueReference>(t);
}
template<typename T> T&& fwd(typename RemoveReference<T>::Type &t) {
    return static_cast<T&&>(t);
}
template<typename T>
void swap(T &a, T &b) {
    T temp = fwd<T&>(a); // or T temp(std::move(t1));
    a = fwd<T&>(b);
    b = fwd<T&>(temp);
}
template<typename U, typename T>
U reinterpret(T t) {
    union {
        T t;
        U u;
    } un = {};
    un.t = t;
    return un.u;
}
} // namespace meta
} // namespace exy

#define Assert(expr) do { if (!(expr)) DebugBreak(); } while (0)
#define OsError(function, msg, ...) Assert(0)
#define UNREACHABLE() do { Assert(0); ExitProcess(0); } while (0)

#define EXY_EXTENSION ".exy"

#define S(text) (text), ((INT)__crt_countof(text) - 1)

#define SIZEOF_BYTE    sizeof(UINT8)
#define SIZEOF_SHORT   sizeof(UINT16)
#define SIZEOF_INT     sizeof(UINT32)
#define SIZEOF_FLOAT   sizeof(float)
#define SIZEOF_DOUBLE  sizeof(double)
#define SIZEOF_POINTER sizeof(void*)

namespace exy {
using DOUBLE = double;
} // namespace exy

constexpr auto cstrlen(const CHAR *v) { return v ? (INT)strlen(v) : 0; }
constexpr auto tmpbufcap = 0x1000;
static thread_local CHAR tmpbuf[tmpbufcap]{};

#include "hash.h"
#include "mem.h"
#include "string.h"
#include "list.h"
#include "dict.h"
#include "console.h"
#include "aio.h"

namespace exy {
template<typename T>
T* ndispose(T *node) {
    if (node != nullptr) {
        node->dispose();
    }
    return nullptr;
}

template<typename T>
void ldispose(List<T*> &list) {
    list.dispose([](T *x) { ndispose(x); });
}

/*template<typename T>
void ldispose(Queue<T> &queue) {
    queue.dispose([](T *x) { ndispose(x); });
}*/

template<typename T>
void ldispose(Dict<T*> &dict) {
    dict.dispose([](T *x) { ndispose(x); });
}

struct Status {
    enum { Idle, Busy, Done } value;

    auto isIdle() const { return value == Idle; }
    auto isBusy() const { return value == Busy; }
    auto isDone() const { return value == Done; }

    auto isNotIdle() const { return value > Idle; }
    auto isNotDone() const { return value < Done; }

    auto begin() { if (value == Idle) { value = Busy; return true; } return false; }
    auto finish() { Assert(value == Busy); value = Done; }
};
} // namespace exy

#include "keywords.h"
#include "identifiers.h"
#include "config.h"
#include "token_kind.h"
#include "token.h"
#include "compiler.h"