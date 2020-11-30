////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-21
////////////////////////////////////////////////////////////////

#include "pch.h"

namespace exy {
Date::Date(const FILETIME &ft) {
    ULARGE_INTEGER ul{};
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    ticks = ul.QuadPart;
}

Date Date::localTime() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    FILETIME ft{};
    SystemTimeToFileTime(&st, &ft);
    return ft;
}

auto Date::systemTime() const -> SYSTEMTIME {
    SYSTEMTIME st{};
    auto ft = fileTime();
    if (!FileTimeToSystemTime(&ft, &st)) {
    }
    return st;
}

auto Date::fileTime() const -> FILETIME {
    ULARGE_INTEGER ul{};
    ul.QuadPart = ticks;
    return { ul.LowPart, ul.HighPart };
}

auto Date::systemTime(const FILETIME &ft) -> SYSTEMTIME {
    SYSTEMTIME st{};
    FileTimeToSystemTime((FILETIME*)&ft, &st);
    return st;
}
} // namespace exy
