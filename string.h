#pragma once

namespace exy {

struct String {
    CHAR *text{};
    INT   length{};
    UINT  hash{};

    String() = default;
    String(const CHAR *v) : text((CHAR*)v), length(cstrlen(v)) {}
    String(const CHAR *v, INT vlen) : text((CHAR*)v), length(vlen) {}
    String(const CHAR *v, INT vlen, UINT vhash) : text((CHAR*)v), length(vlen), hash(vhash) {}
    String(const CHAR *v, const CHAR *vend) : text((CHAR*)v), length(INT(vend - v)) {}
    String(const CHAR *v, const CHAR *vend, UINT vhash) : text((CHAR*)v), length(INT(vend - v)), hash(vhash) {}
    void dispose();

    bool operator==(const String &other) const { return cmp(other) == 0; }
    bool operator!=(const String &other) const { return cmp(other) == 0; }

    INT cmp(const String &other, bool caseSensitive = true) const;

    String& reserve(INT n);
    String& append(const String&);
    String& append(const String*);
    String& append(const CHAR *v, INT vlen);
    String& appendInt(INT n);

    String& removeTrailingSpaces();

    const CHAR* start() const { return text; }
    const CHAR* end()   const { return text + length; }

    bool startsWith(const CHAR *v, INT vlen) const;
    bool endsWith(const CHAR *v, INT vlen)   const;

    auto startsWith(const String &v) const { return startsWith(v.text, v.length); }
    auto endsWith(const String &v)   const { return endsWith(v.text, v.length); }

    auto isEmpty()    const { return length == 0; };
    auto isNotEmpty() const { return length > 0; }

    bool isRandomOf(const CHAR *v, INT vlen) const;
};

using Identifier = const String*;

} // namespace exy