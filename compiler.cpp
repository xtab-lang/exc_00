#include "pch.h"

#include "src.h"
#include "syntax.h"

namespace exy {

void Compiler::run() {
    traceln("Starting compiler");
    ids.initialize();
    if (compiler.config.initialize()) {
        compiler.source = MemNew<SourceTree>();
        if (compiler.source->initialize()) {
            compiler.syntax = MemNew<SyntaxTree>();
            if (compiler.syntax->initialize()) {

            }
        }
    }
    compiler.dispose();
}

void Compiler::dispose() {
    traceln("Stopping compiler");
    if (syntax != nullptr) {
        syntax->dispose();
        syntax = MemFree(syntax);
    }
    if (source != nullptr) {
        source->dispose();
        source = MemFree(source);
    }
    config.dispose();
    ids.dispose();
}

void Compiler::error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
                     const CHAR *pass, const SourceFile &file, const SourceRange &range, const CHAR *msg, ...) {
    va_list ap = nullptr;
    __crt_va_start(ap, msg);
    error(cppFile, cppFunc, cppLine, pass, &file, &range.start, &range.end, msg, ap);
    __crt_va_end(ap);
}

void Compiler::error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
                     const CHAR *pass, const SourcePos &pos, const CHAR *msg, ...) {
    va_list ap = nullptr;
    __crt_va_start(ap, msg);
    error(cppFile, cppFunc, cppLine, pass, &pos.file, &pos.range.start, &pos.range.end, msg, ap);
    __crt_va_end(ap);
}

void Compiler::error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
                     const CHAR *pass, const SourcePos *pos, const CHAR *msg, ...) {
    va_list ap = nullptr;
    __crt_va_start(ap, msg);
    if (pos == nullptr) {
        error(cppFile, cppFunc, cppLine, pass, nullptr, nullptr, nullptr, msg, ap);
    } else {
        error(cppFile, cppFunc, cppLine, pass, &pos->file, &pos->range.start, &pos->range.end, msg, ap);
    }
    __crt_va_end(ap);
}

void Compiler::error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
                     const CHAR *pass, const SourceToken &token, const CHAR *msg, ...) {
    va_list ap = nullptr;
    __crt_va_start(ap, msg);
    error(cppFile, cppFunc, cppLine, pass, &token.pos.file, &token.pos.range.start, &token.pos.range.end, msg, ap);
    __crt_va_end(ap);
}

void Compiler::error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
                     const CHAR *pass, const SourceToken *token, const CHAR *msg, ...) {
    va_list ap = nullptr;
    __crt_va_start(ap, msg);
    if (token == nullptr) {
        error(cppFile, cppFunc, cppLine, pass, nullptr, nullptr, nullptr, msg, ap);
    } else {
        error(cppFile, cppFunc, cppLine, pass, &token->pos.file, 
              &token->pos.range.start, &token->pos.range.end, msg, ap);
    }
    __crt_va_end(ap);
}

void Compiler::error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
                     const CHAR *pass, const SyntaxNode *node, const CHAR *msg, ...) {
    va_list ap = nullptr;
    __crt_va_start(ap, msg);
    if (node == nullptr) {
        error(cppFile, cppFunc, cppLine, pass, nullptr, nullptr, nullptr, msg, ap);
    } else {
        auto &start = node->pos;
        auto   &end = node->lastPos();
        Assert(&start.pos.file == &end.pos.file);
        error(cppFile, cppFunc, cppLine, pass, &node->pos.pos.file, 
              &start.pos.range.start, &end.pos.range.end, msg, ap);
    }
    __crt_va_end(ap);
}

void Compiler::error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
                     const CHAR *pass, const SourceFile *file, const SourceChar *start,
                     const SourceChar *end, const CHAR *msg, va_list ap) {
    ++errors;
    if (pass == nullptr) {
        pass = "Compiler";
    }
    if (file == nullptr) {
        return error(cppFile, cppFunc, cppLine, pass, msg, ap);
    }
    Assert(*start <= *end);
    // '#' error-number '.' file-name '(' start-pos ':' end-pos ')' ':' error-type '→' message
    // highlight
    trace("\r\n#%i#<red>. %s#<yellow underline>(", errors, file->dotName);
    if (start->line == end->line) {
        if (start->col == end->col) {
            trace("%i#<yellow>:%i#<yellow>", start->line, start->col);
        } else {
            trace("%i#<yellow>:%i#<yellow>―%i#<yellow>", start->line, start->col, end->col);
        }
    } else {
        Assert(start->line < end->line);
        trace("%i#<yellow>:%i#<yellow>―%i#<yellow>:%i#<yellow>", start->line, start->col, end->line, end->col);
    }
    trace(") %c#<red underline>%c#<red underline> %c#<darkred> ", pass, "Error", "→");
    if (msg) {
        vprintln(nullptr, msg, ap);
    } else {
        traceln("");
    }
    auto maxLineNumberLength = highlight(file, start, end);
    for (auto i = 0; i < maxLineNumberLength; i++) {
        trace(" ");
    }
    traceln("  %c#<darkyellow>(%c#<darkyellow>:%i#<darkyellow>)", cppFile, cppFunc, cppLine);
}

void Compiler::error(const CHAR *, const CHAR *, INT , 
                     const CHAR *, const CHAR *, va_list) {
    Assert(0);
}
} // namespace exy
