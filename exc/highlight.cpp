////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-20
////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "highlight.h"

#include "console.h"

namespace strm {
void print(const char *fmt, ...) {
    va_list vargs{};
    __crt_va_start(vargs, fmt);
    vprint(fmt, vargs);
    __crt_va_end(vargs);
}

void println(const char *fmt, ...) {
    va_list vargs{};
    __crt_va_start(vargs, fmt);
    vprintln(fmt, vargs);
    __crt_va_end(vargs);
}

void vprintln(const char *fmt, va_list vargs) {
    console::Locker locker{};
    vprint(fmt, vargs);
    print("\r\n");
}

////////////////////////////////////////////////////////////////
static auto take(const char *vstart, const char *vend) {
    console::write(vstart, (int)(vend - vstart));
    return vend;
}

struct Format {
    const char *pos{};
    int group{};
    int width{};
};

static Format parse(const char *pos);
static void write(const char *arg, const Format &specs);
static void write(const char *arg, int arglen, const Format &specs);
static void writei32(int arg, const Format &specs);

void vprint(const char *fmt, va_list vargs) {
    if (!fmt) {
        return;
    }
    Format specs{};
    auto mark = fmt;
    auto  pos = fmt;
    while (*pos) {
        for (; *pos && *pos != '%'; ++pos) {}
        if (!*pos) break; // EOS.
        auto perc = pos++; // {pos} is now past '%'.
        if (!*pos) break; // EOS.
        mark = take(mark, perc);
        switch (auto ch = *pos++) { // {pos} is now past 1st letter of specifier.
            case 'c': {
                if (*pos == 'l') { // '%cl'.
                    specs = parse(++pos);
                    auto    arg = __crt_va_arg(vargs, const char*);
                    auto arglen = __crt_va_arg(vargs, int);
                    write(arg, arglen, specs);
                } else { // '%c'.
                    specs = parse(pos);
                    auto arg = __crt_va_arg(vargs, const char*);
                    write(arg, specs);
                }
            } break;
            case 'i': { // '%i'
                specs = parse(pos);
                auto arg = __crt_va_arg(vargs, int);
                writei32(arg, specs);
            } break;
            default: {
                specs.pos = pos;
            } break;
        }
        mark = specs.pos;
    }
    take(mark, pos);
}

////////////////////////////////////////////////////////////////
static Format parse(const char *pos) {
    Format fmt{};
    auto hash = pos;
    while (*hash == '#') {
        ++pos; // Past '#'.
        if (*pos) {
            break; // EOS.
        } switch (*pos) {
            case '<': { // '#' '<' [fore-color] ['|' back-color] '>'
            } break;
            default: {
                pos = hash;
            } break;
        }
        hash = pos;
    }
    fmt.pos = hash;
    return fmt;
}

void write(const char *arg, const Format &specs) {
    write(arg, cstrlen(arg), specs);
}

void write(const char *arg, int arglen, const Format &) {
    console::write(arg, arglen);
}

void writei32(int arg, const Format &specs) {
    _set_errno(0);
    if (auto err = _itoa_s(arg, numbuf, 10)) {
    }
    auto length = cstrlen(numbuf);
    write(numbuf, length, specs);
}

}