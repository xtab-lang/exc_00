#include "pch.h"
#include "exc.h"

int main(INT, const CHAR**) {
    for (auto i = 0; i < 10; ++i) {
        traceln("The %c#<yellow underline> language compiler (%c#<bold>).", "exy", "exc");
        exy::heap::initialize();
        if (exy::aio::open()) {
        }
        exy::aio::close();
        exy::heap::dispose();
    }
    return 0;
}