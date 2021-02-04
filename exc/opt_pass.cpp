//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-01
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "opt_pass.h"

namespace exy {
namespace opt_pass {
bool run() {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, threads: %u#<green> }",
        S("optimizer"), S("optimize ir"), aio::ioThreads());

    traceln("%cl#<cyan|blue> { errors: %i#<red> }",
        S("optimizer"), comp.errors);
    return comp.errors == 0;
}
} // namespace opt
} // namespace exy