////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-20
////////////////////////////////////////////////////////////////
#pragma once
#ifndef CONSOLE_H_
#define CONSOLE_H_

namespace console {
struct Locker {
    bool shouldRelease{};
    Locker();
    ~Locker();
};

void write(const char *v, int vlen);
void write(const char *v);
void writeln(const char *v, int vlen);
void writeln(const char *v);
} // namespace console

#endif // CONSOLE_H_