////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

module;
#include "stdafx.h"
export module string;

import hash;

export struct String final {
    const char  *text   = nullptr;
    int		     length = 0;
    unsigned int hash   = 0ui32;

    String() {}
    String(const char *text) : text(text), length(cstrlen(text)), hash(hash32(text, cstrlen(text))) {}
    String(const char *text, int length) : text(text), length(length), hash(hash32(text, length)) {}
};

using Identifier = const String*;