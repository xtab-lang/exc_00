////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-23
////////////////////////////////////////////////////////////////

#include "pch.h"

namespace exy {
String::String(const char *v, const char *vend) : String(v, (int)(vend - v)) {}

String::String(const char *text, int length) : text((char*)text), length(length), hash(hash32(text, length)) {}

String::String(const char *text, int length, unsigned int hash) : text((char*)text), length(length), hash(hash) {}

bool String::operator==(const String &other) const {
    return cmp(other) == 0;
}

bool String::operator!=(const String &other) const {
    return cmp(other) != 0;
}

bool String::operator==(const char *v) const {
    return cmp(v) == 0;
}

bool String::operator!=(const char *v) const {
    return cmp(v) != 0;
}

int String::cmp(const String &other, bool caseSensitive) const {
    return cmp(other.text, other.length, caseSensitive);
}

int String::cmp(const char *v, bool caseSensitive) const {
    if (!text || !length) {
        return !v ? 0 : -1;
    } if (!v) {
        return 1;
    } if (caseSensitive) {
        return strcmp(text, v);
    }
    return _stricmp(text, v);
}

int String::cmp(const char *v, int vlen, bool caseSensitive) const {
    if (!text || !length) {
        return !v ? 0 : -1;
    } if (!v || !vlen) {
        return 1;
    } if (length > vlen) {
        return 1;
    } if (length < vlen) {
        return -1;
    } if (caseSensitive) {
        return strncmp(text, v, vlen);
    }
    return _strnicmp(text, v, vlen);
}

bool String::isEmpty() const {
    return length == 0;
}
} // namespace exy