//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-01
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "linker_pass.h"

#include "binary.h"

#define err(token, msg, ...) print_error("Linker", token, msg, __VA_ARGS__)

namespace exy {
namespace linker_pass {
struct LinkerModuleConsumer {
    auto next(IrModule *node) {
        traceln("linking %cl#<cyan> %s#<green> @thread(%u#<green>)",
                S("module"), node->name, GetCurrentThreadId());
        Binary binary{ node };
        binary.generate();
        binary.dispose();
    }
};

bool run() {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, threads: %u#<green> }",
        S("linker"), S("link modules"), aio::ioThreads());

    ModuleProvider provider{};
    LinkerModuleConsumer consumer{};
    aio::run(consumer, provider);

    traceln("%cl#<cyan|blue> { errors: %i#<red> }",
        S("linker"), comp.errors);
    return comp.errors == 0;
}
} // namespace opt
} // namespace exy