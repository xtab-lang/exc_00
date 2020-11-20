////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

module;
#include "stdafx.h"
#include <wincon.h>
export module console;

import string;
import hash;

export struct Console {
    SRWLOCK  srw{};
    static thread_local int locks;

    void write(const char *text, int length) {
        if (!text) {
            return;
        } if (!length) {
            length = cstrlen(text);
            if (!length) {
                return;
            }
        }
        lock();
        auto out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD written = 0;
        auto res = WriteConsole(out, text, length, &written, nullptr);
        if (res == FALSE) { // Failed.

        } // Else succeeded.
        unlock();
    }

    void write(const char *text) {
        write(text, cstrlen(text));
    }
    
    void writeln(const char *text, int length) {
        lock();
        write(text, length);
        write(S("\r\n"));
        unlock();
    }

    void writeln(const char *text) {
        writeln(text, cstrlen(text));
    }

private:
    void lock() {
        if (++locks == 1) {
            AcquireSRWLockExclusive(&srw);
        }
    }
    void unlock() {
        if (--locks == 0) {
            ReleaseSRWLockExclusive(&srw);
        }
    }
};

thread_local int Console::locks{};
export Console console{};