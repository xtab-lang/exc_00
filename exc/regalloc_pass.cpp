//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-01
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "regalloc_pass.h"

#include "ir.h"

namespace exy {
namespace regalloc_pass {
struct RegAllocModuleConsumer {
    auto next(IrModule *mod) {
        traceln("allocating %cl#<cyan> %s#<green> @thread(%u#<green>)",
            S("module"), mod->name, GetCurrentThreadId());
    }
};

bool run() {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, threads: %u#<green> }",
        S("regalloc"), S("allocate registers"), aio::ioThreads());

    ModuleProvider provider{};
    RegAllocModuleConsumer consumer{};
    aio::run(consumer, provider);

    traceln("%cl#<cyan|blue> { errors: %i#<red> }",
        S("regalloc"), comp.errors);
    return comp.errors == 0;
}
} // namespace regalloc
} // namespace exy