//////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-22
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

#pragma once
#ifndef PCH_H_
#define PCH_H_

#define _AMD64_
//#define _M_AMD64
//#define _X86_

#undef _INC_WINDOWS
#include <winapifamily.h>
#include <intrin.h>
#include <sdkddkver.h>
#include <windef.h>
#include <WinBase.h>
#include <stdlib.h>
#include <new>

#pragma warning(disable: 26495)

/* warning C4201: nonstandard extension used: nameless struct/union */
//#pragma warning(disable: 4201)

/* warning 4706: assignment within conditional expression */
//#pragma warning(disable: 4706)

/* warning 4706: assignment within conditional expression */
//#pragma warning(disable: 26812)

/* warning 4706: assignment within conditional expression */
//#pragma warning(disable: 26495)

/* unscoped enum blah! blah! blah! */
#pragma warning(disable: 26812)

#define S(text) (text), ((int)__crt_countof(text) - 1)

constexpr auto cstrlen(const char *text) { return text ? (int)strlen(text) : 0; }

constexpr int numbufcap = 1024;
static thread_local char numbuf[numbufcap];

constexpr int SizeOfZmm     = 0x40;
constexpr int SizeOfYmm     = 0x20;
constexpr int SizeOfXmm     = 0x10;
constexpr int SizeOfDouble  = sizeof(double);
constexpr int SizeOfFloat   = sizeof(float);
constexpr int SizeOfPointer = sizeof(void*);
constexpr int SizeOfInt     = sizeof(INT32);
constexpr int SizeOfWord    = sizeof(INT16);
constexpr int SizeOfByte    = sizeof(INT8);

template<typename T>
struct Atomic {
    SRWLOCK srw{};
    T       value{};

    T operator++() {
        auto result = value;
        AcquireSRWLockExclusive(&srw);
        result = value++;
        ReleaseSRWLockExclusive(&srw);
        return result;
    }

    T operator--() {
        auto result = value;
        AcquireSRWLockExclusive(&srw);
        result = value--;
        ReleaseSRWLockExclusive(&srw);
        return result;
    }
};

#include "lib.h"
#include "compiler.h"
#include "identifiers.h"

#endif // PCH_H_