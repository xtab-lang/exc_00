////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-21
////////////////////////////////////////////////////////////////

#pragma once
#ifndef DATE_H_
#define DATE_H_

namespace exy {
struct Date {
    unsigned __int64 ticks;

    Date() : ticks(0) {}
    Date(unsigned __int64 ticks) : ticks(ticks) {}
    Date(const FILETIME &ft);

    static Date localTime();

    auto systemTime() const -> SYSTEMTIME;
    auto fileTime() const -> FILETIME;

    static auto systemTime(const FILETIME &ft) -> SYSTEMTIME;
    static auto systemTime(const Date &dt) -> SYSTEMTIME;
    static auto systemTime(const unsigned __int64 ticks) -> SYSTEMTIME;
    static auto fileTime(const SYSTEMTIME &st) -> FILETIME;
    static auto fileTime(const Date &dt) -> FILETIME;
    static auto fileTime(const unsigned __int64 ticks) -> FILETIME;
}; // struct Date
} // namespace exy

#endif // DATE_H_