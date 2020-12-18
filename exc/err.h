////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-21
////////////////////////////////////////////////////////////////

#pragma once
#ifndef ERR_H_
#define ERR_H_

namespace exy {
#define Assert(expr) do { if (!(expr)) DebugBreak(); } while (0)
#define OsError(osFunction, userMsg, ...) formatOsError(GetLastError(), osFunction, userMsg, __VA_ARGS__)
#define Unreachable() do { Assert(0); ExitProcess(0); } while (0)

void formatOsError(DWORD code, const char *osFunction, const char *msg, ...);
} // namespace exy

#endif // ERR_H_