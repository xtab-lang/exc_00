#include "pch.h"
#include "src.h"

namespace exy {

void Compiler::run() {
    traceln("Starting compiler");
    ids.initialize();
    if (compiler.config.initialize()) {
        compiler.source = MemNew<SourceTree>();
        if (compiler.source->initialize()) {

        }
    }
    compiler.dispose();
}

void Compiler::dispose() {
    traceln("Stopping compiler");
    if (source != nullptr) {
        source->dispose();
        source = MemFree(source);
    }
    config.dispose();
    ids.dispose();
}

} // namespace exy
