////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

#include "pch.h"
#include "compiler.h"

int main(int, char**) {
    for (auto i = 0; i < 10; ++i) {
        lib::start();
        Compiler compiler{};
        compiler.run(i);
        compiler.dispose();
        lib::stop();
    }
    return 0;
}