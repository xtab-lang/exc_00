////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-22
////////////////////////////////////////////////////////////////

#include "pch.h"
#include "compiler.h"

#include "identifiers.h"
#include "source.h"
#include "tokenizer.h"
#include "syntax.h"

#define ROOT_FOLDER                 "D:\\c\\exc\\source"
#define MAX_FILE_SIZE               0x10000
#define DEFAULT_FILES_PER_THREAD    1

namespace exy {
//------------------------------------------------------------------------------------------------
String SourceToken::name() const { 
    switch (kind) {
    #define ZM(zName, zText) case Tok::zName: return { S(#zName), 0u };
        DeclarePunctuationTokens(ZM)
        DeclareGroupingTokens(ZM)
        DeclareOperatorTokens(ZM)
        DeclareTextTokens(ZM)
    #undef ZM
    default:
        Assert(0);
        break;
}
return {};
}
String SourceToken::value() const {
    if (kind >= Tok::Text) {
        return range.value();
    } switch (kind) {
    #define ZM(zName, zText) case Tok::zName: return { S(zText), 0u };
        DeclarePunctuationTokens(ZM)
        DeclareGroupingTokens(ZM)
        DeclareOperatorTokens(ZM)
        DeclareTextTokens(ZM)
    #undef ZM
        default:
            Assert(0);
            break;
    }
    return {};
}
//------------------------------------------------------------------------------------------------
void Compiler::run() {
    options.path.append(ROOT_FOLDER);
    options.maxFileSize           = MAX_FILE_SIZE;
    options.defaultFilesPerThread = DEFAULT_FILES_PER_THREAD;

    if (src_pass::run()) {
        if (tok_pass::run()) {
            if (syn_pass::run()) {
                /*if (typ_pass::run(*this)) {
                    if (irg_pass::run(*this)) {

                    }
                }*/
            }
        }
    }
    if (syntax) {
        syntax->dispose();
        syntax = memfree(syntax);
    }
    if (source) {
        source->dispose();
        source = memfree(source);
    }

    options.path.dispose();
}

namespace comp_pass {
void run(int compId) {
    traceln("%cl#<underline yellow> is the %cl#<underline red> language compiler", S("exc"), S("exy"));
    traceln("Compid: %i#<magenta>", compId);

    ids = Identifiers{};
    ids.initialize();

    comp = Compiler{};
    comp.run();

    ids.dispose();
}
} // namespace comp_pass
} // namespace exy
