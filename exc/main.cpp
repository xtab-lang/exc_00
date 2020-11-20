////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

#include "stdafx.h"

import lib;

int main(int argc, char **argv) {
    console.writeln(S("The exy compiler"));
    if (argv) {
        for (auto i = 0; i < argc; ++i) {
            console.writeln(argv[i], cstrlen(argv[i]));
        }
    } else {
        console.writeln("No command line arguments.");
    }
    return 0;
}