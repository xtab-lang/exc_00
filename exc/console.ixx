////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

module;
#include "stdafx.h"
#include <wincon.h>
export module lib:console;

import :string;
import :hash;

namespace console {

static SRWLOCK   srw{};
thread_local int locks{};

void lockConsole() {
    if (++locks == 1) {
        AcquireSRWLockExclusive(&srw);
    }
}

void unlockConsole() {
    if (--locks == 0) {
        ReleaseSRWLockExclusive(&srw);
    }
}

export void write(const char *text, int length) {
    if (!text) {
        return;
    } if (!length) {
        length = cstrlen(text);
        if (!length) {
            return;
        }
    }
    lockConsole();
    auto out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    auto res = WriteConsole(out, text, length, &written, nullptr);
    if (res == FALSE) { // Failed.

    } // Else succeeded.
    unlockConsole();
}

export void writeln(const char *text, int length) {
    lockConsole();
    write(text, length);
    write(S("\r\n"));
    unlockConsole();
}

export void writeln(const char *text) {
    lockConsole();
    writeln(text, cstrlen(text));
    unlockConsole();
}

export void write(const String &str) {
    lockConsole();
    write(str.text, str.length);
    unlockConsole();
}

export void writeln(const String &str) {
    lockConsole();
    write(str);
    write(S("\r\n"));
    unlockConsole();
}

export void write(const String *str) {
    if (!str) {
        return;
    }
    lockConsole();
    write(str->text, str->length);
    unlockConsole();
}

export void writeln(const String *str) {
    if (!str) {
        return;
    }
    lockConsole();
    write(str);
    write(S("\r\n"));
    unlockConsole();
}

}