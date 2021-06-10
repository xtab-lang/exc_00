#pragma once

namespace exy {
struct Identifiers {
    void initialize();
    void dispose();

    Identifier get(const CHAR *v, INT vlen, UINT vhash);
    Identifier get(const CHAR *v, INT vlen);
    Identifier get(const CHAR *v, const CHAR *vend);
    Identifier get(const String&);
    Identifier get(Identifier);

    Identifier random(const CHAR *prefix, INT prefixlen);
private:
    Mem              mem{};
    Dict<Identifier> list{};
    SRWLOCK          srw{};
    INT              randomCounter = 1000;

    void lock() { AcquireSRWLockExclusive(&srw); }
    void unlock() { ReleaseSRWLockExclusive(&srw); }
    Identifier append(const CHAR *v, INT vlen, UINT vhash);
};

__declspec(selectany) Identifiers ids{};
} // namespace exy