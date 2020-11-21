////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-20
////////////////////////////////////////////////////////////////
#pragma once
#ifndef HIGHLIGHT_H_
#define HIGHLIGHT_H_

namespace strm {
void print(const char *fmt, ...);
void vprint(const char *fmt, va_list vargs);
void println(const char *fmt, ...);
void vprintln(const char *fmt, va_list vargs);
}

#define trace(fmt, ...) strm::print((fmt), __VA_ARGS__)
#define traceln(fmt, ...) strm::println((fmt), __VA_ARGS__)

#endif // HIGHLIGHT_H_