////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-22
////////////////////////////////////////////////////////////////

#include "pch.h"
#include "compiler.h"

void Compiler::dispose() {
}

void Compiler::run(int id) {
    traceln("%cl#<underline yellow> is the %cl#<underline red> language compiler", S("exc"), S("exy"));
    traceln("Run id: %i#<magenta>", id);
}
