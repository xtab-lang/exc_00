//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-08
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "src2tok_pass.h"

#include "tokenizer.h"
#include "preparser.h"

#include "source.h"

namespace exy {
namespace src2tok_pass {
static void updateTokenCount(SourceFolder *folder) {
    for (auto i = 0; i < folder->folders.length; ++i) {
        updateTokenCount(folder->folders.items[i]);
    } for (auto i = 0; i < folder->files.length; ++i) {
        auto file = folder->files.items[i];
        comp.source->tokens += file->tokens.length;
    }
}

bool run() {
    traceln("\r\n%cl#<cyan|blue> { filesPerThread: %i#<magenta>, threads: %i#<magenta> }",
            S("tokenizer"), comp.options.defaultFilesPerThread, aio::ioThreads());

    SourceFileProvider provider{};
    Tokenizer          tokenizer{};
    //provider.perBatch = aio::ioThreads();
    aio::run(tokenizer, provider);
    provider.dispose();

    if (comp.errors == 0) {
        preparse();
        updateTokenCount(comp.source->root);
    }

    traceln("%cl#<cyan|blue> { errors: %i#<red>, totalTokens: %i#<magenta> }", 
            S("tokenizer"), comp.errors, comp.source->tokens);

    return comp.errors == 0;
}
} // namespace src2tok_pass
} // namespace exy