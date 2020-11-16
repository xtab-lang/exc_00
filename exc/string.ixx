////////////////////////////////////////////////////////////////
// author: munenedu@gmail.com
//   date: 2020-11-16
////////////////////////////////////////////////////////////////
module;
#include "stdafx.h"
export module string;

export struct String final {
    const char  *text;
    int		     length;
    unsigned int hash;
};