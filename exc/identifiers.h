////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-02
////////////////////////////////////////////////////////////////

#pragma once
#ifndef IDS_H_
#define IDS_H_

namespace exy {
struct Identifiers {
    Identifier main;    // 'main'
    Identifier block;   // 'block'
    Identifier entry;   // 'entry'
    Identifier exit;    // 'exit'
    struct {
        Identifier text;    // '.text'
        Identifier data;    // '.data'
        Identifier idata;   // '.idata'
        Identifier edata;   // '.edata'
        Identifier string;  // '.string'
        Identifier stack;   // '.stack'
    } dot;

    void initialize();
    void dispose();

    Identifier get(const char *v, int vlen, UINT vhash);
    Identifier get(const char *v, int vlen);
    Identifier get(const char *v, const char *vend);
    Identifier get(const String&);
    Identifier get(Identifier);
private:
    Mem              mem{};
    Dict<Identifier> list{};
    SRWLOCK          srw{};

    void lock()   { AcquireSRWLockExclusive(&srw); }
    void unlock() { ReleaseSRWLockExclusive(&srw); }
    Identifier append(const char *v, int vlen, UINT vhash);
};

__declspec(selectany) Identifiers ids;

} // namespace exy

#endif // IDS_H_