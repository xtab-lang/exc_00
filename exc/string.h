//////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-23
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

#pragma once
#ifndef STRING_H_
#define STRING_H_

namespace exy {
struct String {
    char *text{};
    int   length{};
    unsigned int hash{};

    String() {}
    String(const char *v, const char *vend);
    String(const char *text, int length);
    String(const char *text, int length, unsigned int hash);

    bool operator==(const String &other) const;
    bool operator!=(const String &other) const;

    bool operator==(const char *v) const;
    bool operator!=(const char *v) const;

    int cmp(const String &other, bool caseSensitive = true) const;
    int cmp(const char *v, bool caseSensitive = true) const;
    int cmp(const char *v, int vlen, bool caseSensitive = true) const;

    bool isEmpty() const;
};

using Identifier = const String*;

} // namespace exy

#endif // STRING_H_