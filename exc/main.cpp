#include "stdafx.h"

import console;
import string;

int main(int argc, char **argv) {
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