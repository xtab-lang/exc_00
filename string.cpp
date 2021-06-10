#include "pch.h"

namespace exy {
void String::dispose() {
    text = MemFree(text);
    length = 0;
    hash = 0u;
}

INT String::cmp(const String &other, bool caseSensitive) const {
    if (length < other.length) {
        return -1;
    }
    if (length > other.length) {
        return 1;
    }
    if (length == 0) {
        return 0;
    }
    if (caseSensitive) {
        return strncmp(text, other.text, length);
    }
    return _strnicmp(text, other.text, length);
}

String& String::reserve(INT n) {
    if (n > 0) {
        auto cap = length + n;
        text = MemReAlloc(text, cap + 1);
    } else {
        Assert(n == 0);
    }
    return *this;
}

String& String::append(const String &v) {
    append(v.text, v.length);
    return *this;
}

String& String::append(const String *v) {
    if (v != nullptr) {
        append(v->text, v->length);
    }
    return *this;
}

String& String::append(const CHAR *v, INT vlen) {
    if (vlen > 0) {
        reserve(vlen);
        MemCopy(text + length, v, vlen);
        length += vlen;
    }
    return *this;
}

String& String::appendInt(INT n) {
    static thread_local CHAR numbuf[0x20]{};
    auto e = _itoa_s(n, numbuf, 10);
    Assert(e != 0); 
    return append(numbuf, cstrlen(numbuf));
}

bool String::startsWith(const CHAR *v, INT vlen) const {
    if (length == 0 || vlen == 0) {
        return false;
    }
    if (vlen > length) {
        return false;
    }
    return strncmp(text, v, vlen) == 0;
}

bool String::endsWith(const CHAR *v, INT vlen) const {
    if (length == 0 || vlen == 0) {
        return false;
    }
    if (vlen > length) {
        return false;
    }
    auto start = length - vlen;
    return strncmp(text + start, v, vlen) == 0;
}
} // namespace exy