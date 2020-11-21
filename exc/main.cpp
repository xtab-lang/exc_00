////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "console.h"
#include "highlight.h"

int main(int argc, char **argv) {
    traceln("exc: The exy language\r\nCommand line arguments:");
    for (auto i = 0; i < argc; ++i) {
        traceln("\t%c", argv[i]);
    }
    return 0;
}