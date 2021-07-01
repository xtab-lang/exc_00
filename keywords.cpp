#include "pch.h"

namespace exy {
void Keywords::initialize() {
#define ZM(zName, zText) list.place(String{ S(zText) }, Keyword::zName);
    DeclareKeywords(ZM)
    DeclareModifiers(ZM)
    DeclareUserDefinedTypeKeywords(ZM)
    DeclareCompilerKeywords(ZM)
#undef ZM
#define ZM(zName, zSize) list.place(String{ S(#zName) }, Keyword::zName);
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
}

void Keywords::dispose() {
    list.dispose();
}

Keyword Keywords::get(const String &str) {
    for (auto i = 0; i < list.length; i++) {
        auto &word = list.items[i];
        if (word.name == str) {
            return word.value;
        }
    }
    return Keyword::None;
}
} // namespace exy