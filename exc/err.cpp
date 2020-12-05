////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-21
////////////////////////////////////////////////////////////////

#include "pch.h"

namespace exy {
void formatOsError(DWORD code, const char *osFunction, const char *userMsg, ...) {
    trace("%c#<red> @ %c#<underline cyan> with %i#<magenta>: ", "OsError", osFunction, code);
    if (userMsg) {
        va_list vargs{};
        __crt_va_start(vargs, userMsg);
        vprintln(nullptr, userMsg, vargs);
        __crt_va_end(vargs);
    } else {
        traceln("Failed to %c ", osFunction);
    }

    char *buffer{};
    auto res = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             nullptr,
                             code,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             (LPTSTR)&buffer,
                             0,
                             nullptr);
    if (res == FALSE) {
        Assert(0);
    } else {
        trace("  %u#<red>:  %cl", code, buffer, cstrlen(buffer));
    }
    LocalFree(buffer);
}
} // namespace exy
