//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-02
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "syntax.h"

#include "parser.h"
#include "source.h"
#include "compiler.h"

namespace exy {
//------------------------------------------------------------------------------------------------
void SyntaxTree::dispose() {
    ldispose(files);
    mem.dispose();
}
//------------------------------------------------------------------------------------------------
void SyntaxFile::dispose() {
    nodes.dispose([](auto *x) { x->dispose(); });
}

const SourceFile& SyntaxFile::sourceFile() const {
    return *pos.file;
}

const List<SourceToken>& SyntaxFile::tokens() const {
    return sourceFile().tokens;
}
//------------------------------------------------------------------------------------------------
void SyntaxModule::dispose() {
    name = ndispose(name);
}
//------------------------------------------------------------------------------------------------
void SyntaxImportOrExport::dispose() {
    name = ndispose(name);
    from = ndispose(from);
    as = ndispose(as);
}
//------------------------------------------------------------------------------------------------
void SyntaxCommaList::dispose() {
    ldispose(nodes);
}
//------------------------------------------------------------------------------------------------
void SyntaxDotExpression::dispose() {
    lhs = ndispose(lhs);
    rhs = ndispose(rhs);
}
//------------------------------------------------------------------------------------------------
SyntaxFileProvider::SyntaxFileProvider() : WorkProvider(comp.options.defaultFilesPerThread),
    files(comp.syntax->files) {}

void SyntaxFileProvider::dispose() {
    pos = 0;
}

bool SyntaxFileProvider::next(List<SyntaxFile*> &batch) {
    AcquireSRWLockExclusive(&srw);
    auto end = min(pos + perBatch, files.length);
    for (; pos < end; ++pos) {
        batch.append(files.items[pos]);
    }
    ReleaseSRWLockExclusive(&srw);
    return batch.isNotEmpty();
}
//------------------------------------------------------------------------------------------------
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

struct Parser {
    auto next(SyntaxFile &file) {
        parse(file);
    }
};

bool run() {
    traceln("\r\n%cl#<cyan|blue> { filesPerThread: %i#<magenta>, threads: %i#<magenta> }",
            S("parser"), comp.source->files, comp.source->bytes, comp.options.defaultFilesPerThread,
            aio::ioThreads());

    comp.syntax = memalloc<SyntaxTree>();
    comp.syntax = new(comp.syntax) SyntaxTree{};

    {
        SourceFileProvider provider{};
        createSyntaxFiles(provider.files);
        provider.dispose();
    } {
        SyntaxFileProvider provider{};
        Parser             parser{};
        aio::run(parser, provider);
        provider.dispose();
    }

    traceln("%cl#<cyan|blue> { }", S("parser"));

    return comp.errors == 0;
}
} // namespace syn_pass
} // namespace exy