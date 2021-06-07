#include "pch.h"
#include "exc.h"

int main(INT, const CHAR**) {
    for (auto i = 0; i < 1; ++i) {
        traceln("The %c#<yellow underline> language compiler (%c#<bold>).", "exy", "exc");
        exy::heap::initialize();
        if (exy::aio::open()) {
            exy::Compiler::run();
        }
        exy::aio::close();
        exy::heap::dispose();
    }
    return 0;
}