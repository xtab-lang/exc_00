////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

#include "stdafx.h"

import common;

int main(int argc, char **argv) {
    String s{ S("hello") };
    console::writeln(S("Hello c++ modules!"));
    if (argv) {
        for (auto i = 0; i < argc; ++i) {
            console::writeln(argv[i], cstrlen(argv[i]));
        }
    } else {
        console::writeln("No command line arguments.");
    }
    return 0;
}