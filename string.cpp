#include "pch.h"

namespace exy {
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
} // namespace exy