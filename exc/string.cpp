////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-23
////////////////////////////////////////////////////////////////

#include "pch.h"

namespace exy {
String::String(const char *v, const char *vend) : String(v, (int)(vend - v)) {}

String::String(const char *v, const char *vend, unsigned int vhash) : String(v, (int)(vend - v), vhash) {}

String::String(const char *text, int length) : text((char*)text), length(length), hash(hash32(text, length)) {}

String::String(const char *text, int length, unsigned int hash) : text((char*)text), length(length), hash(hash) {}

void String::dispose() {
    text = memfree(text);
    length = 0;
    hash = 0u;
}

String& String::clear() {
    length = 0;
    hash = 0u;
    return *this;
}

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

const char* String::end() const {
    return text + length;;
}

String& String::reserve(int size) {
    if (size > 0) {
        text = memrealloc(text, size);
    }
    return *this;
}

String& String::append(const String &other) {
    return append(other.text, other.length);
}

String& String::append(const String *other) {
    if (other) {
        return append(other->text, other->length);
    }
    return *this;
}

String& String::append(const char *v) {
    return append(v, cstrlen(v));
}

String& String::append(const char *v, int vlen) {
    if (v && vlen > 0) {
        auto len = length + vlen;
        text = memrealloc(text, len + 1);
        MemCopy(text + length, v, vlen);
        text[length = len] = '\0';
    }
    return *this;
}

String String::getFullFileName() const {
    if (text && length > 0) {
        auto pos = text + length;
        for (--pos; pos > text; --pos) {
            if (*pos == '\\' || *pos == '/') {
                return { pos + 1, text + length, 0u };
            }
        }
    }
    return {};
}

String String::getFileName() const {
    if (text && length > 0) {
        auto dot = text + length;
        for (--dot; dot > text && *dot != '.'; --dot);
        if (dot == text) {
            dot = text + length;
        }
        auto pos = dot;
        for (--pos; pos > text; --pos) {
            if (*pos == '\\' || *pos == '/') {
                return { pos + 1, dot, 0u };
            }
        }
        return { text, dot, 0u };
    }
    return {};
}

String String::getFileExtension() const {
    if (text && length > 0) {
        auto pos = text + length;
        for (--pos; pos > text; --pos) {
            if (*pos == '\\' || *pos == '/') {
                break;
            } if (*pos == '.') {
                return { pos + 1, text + length, 0u };
            }
        }
    }
    return {};
}

} // namespace exy