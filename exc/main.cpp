////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

#include "pch.h"
#include "compiler.h"

int main(int, char**) {
    for (auto i = 0; i < 1; ++i) {
        lib::start();
        exy::comp_pass::run(i);
        lib::stop();
    }
    return 0;
}