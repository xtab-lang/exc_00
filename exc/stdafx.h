////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

#pragma once
#ifndef STDAFX_H_
#define STDAFX_H_

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

#define S(text) (text), (int)__crt_countof(text)

constexpr auto cstrlen(const char *text) { return text ? (int)strlen(text) : 0; }

template<typename T>
constexpr void memzero(T *mem, int count = 1) { ZeroMemory(mem, sizeof(T) * count); }

template<typename T>
constexpr void memcopy(T *dst, T *src, int count = 1) { CopyMemory(dst, src, sizeof(T) * count); }

template<typename T>
constexpr void memmove(T *dst, T *src, int count = 1) { MoveMemory(dst, src, sizeof(T) * count); }

static thread_local char numbuf[_CVTBUFSIZE];

#endif // STDAFX_H_