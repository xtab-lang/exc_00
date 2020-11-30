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

/* warning C4201: nonstandard extension used: nameless struct/union */
//#pragma warning(disable: 4201)

/* warning 4706: assignment within conditional expression */
//#pragma warning(disable: 4706)

/* warning 4706: assignment within conditional expression */
//#pragma warning(disable: 26812)

/* warning 4706: assignment within conditional expression */
//#pragma warning(disable: 26495)

#define S(text) (text), ((int)__crt_countof(text) - 1)

constexpr auto cstrlen(const char *text) { return text ? (int)strlen(text) : 0; }

template<typename T>
constexpr void memzero(T *mem, int count = 1) { ZeroMemory(mem, sizeof(T) * count); }

template<typename T, typename U>
constexpr void memcopy(T *dst, U *src, int count = 1) { CopyMemory((void*)dst, (void*)src, sizeof(T) * count); }

template<typename T, typename U>
constexpr void memmove(T *dst, U *src, int count = 1) { MoveMemory(dst, src, sizeof(T) * count); }

constexpr int numbufcap = 1024;
static thread_local char numbuf[numbufcap];

#include "lib.h"

#endif // PCH_H_