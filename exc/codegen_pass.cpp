//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-01
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "codegen_pass.h"

#include "ir2pe.h"

namespace exy {
namespace codegen_pass {
//------------------------------------------------------------------------------------------------
struct ModuleConsumer {
    auto next(IrModule *node) {
        Module mod{ node };
        mod.visit();
        mod.dispose();
    }
};

bool run() {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, threads: %u#<green> }",
        S("codegen"), S("generate code"), aio::ioThreads());

    ModuleProvider provider{};
    ModuleConsumer consumer{};
    aio::run(consumer, provider);

    traceln("%cl#<cyan|blue> { errors: %i#<red> }",
        S("codegen"), comp.errors);
    return comp.errors == 0;
}
} // namespace opt
} // namespace exy