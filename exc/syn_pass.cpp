//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-09
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "syn_pass.h"

#include "parser.h"

#include "source.h"
#include "syntax.h"

namespace exy {
namespace syn_pass {
static void createSyntaxFiles(List<SourceFile*> &files) {
    auto syntax = comp.syntax;
    auto   &mem = syntax->mem;
    for (auto i = 0; i < files.length; ++i) {
        auto sourceFile = files.items[i];
        Assert(sourceFile->tokens.length);
        auto       &pos = sourceFile->tokens.first();
        auto syntaxFile = mem.New<SyntaxFile>(pos);
        syntax->files.append(syntaxFile);
    }
}

struct SyntaxFileParser {
    auto next(SyntaxFile &file) {
        auto &sourceFile = file.sourceFile();
        traceln("%s#<underline yellow> { file: %s#<underline cyan>, tokens: %i#<magenta>, thread: %i#<green> }",
                sourceFile.path, sourceFile.dotName, sourceFile.tokens.length, GetCurrentThreadId());
        Parser parser{ file.tokens(), comp.syntax->mem };
        parser.parseFile(file);
        parser.dispose();
    }
};

bool run() {
    traceln("\r\n%cl#<cyan|blue> { filesPerThread: %i#<magenta>, threads: %i#<magenta> }",
            S("parser"), comp.options.defaultFilesPerThread, aio::ioThreads());

    comp.syntax = MemAlloc<SyntaxTree>(); 
    {
        SourceFileProvider provider{};
        createSyntaxFiles(provider.files);
        provider.dispose();
    } {
        SyntaxFileProvider provider{};
        SyntaxFileParser   parser{};
        //provider.perBatch = aio::ioThreads();
        aio::run(parser, provider);
        provider.dispose();
    }

    traceln("%cl#<cyan|blue> { errors: %i#<red> }", S("parser"), comp.errors);

    return comp.errors == 0;
}
} // namespace syn_pass
} // namespace exy