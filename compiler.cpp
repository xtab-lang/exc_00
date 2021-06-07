#include "pch.h"

namespace exy {

void Compiler::run() {
    Assert(compiler == nullptr);
    traceln("Starting compiler");
    compiler = MemNew<Compiler>();
    if (compiler->config.initialize()) {

    }
    compiler->dispose();
}

void Compiler::dispose() {
    traceln("Stopping compiler");
    config.dispose();
    compiler = MemFree(compiler);
}

} // namespace exy
