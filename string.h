#pragma once

namespace exy {

struct String {
    CHAR  *text{};
    INT    length{};
    UINT32 hash{};

    String() = default;
    String(const CHAR *v) : text((CHAR*)v), length(cstrlen(v)) {}
    String(const CHAR *v, INT vlen) : text((CHAR*)v), length(vlen) {}
    String(const CHAR *v, const CHAR *vend) : text((CHAR*)v), length(INT(vend - v)) {}

    bool operator==(const String &other) const { return cmp(other) == 0; }
    bool operator!=(const String &other) const { return cmp(other) == 0; }

    INT cmp(const String &other, bool caseSensitive = true) const;
};

using Identifier = const String*;

} // namespace exy