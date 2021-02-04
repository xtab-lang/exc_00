////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-22
////////////////////////////////////////////////////////////////

#include "pch.h"

#include "src_pass.h"
#include "src2tok_pass.h"
#include "syn_pass.h"
#include "stx2ast_pass.h"
#include "ast2ir_pass.h"
#include "opt_pass.h"
#include "regalloc_pass.h"
#include "codegen_pass.h"
#include "linker_pass.h"

#include "source.h"
#include "typer.h" // Includes "ast.h"
#include "ir.h"

#define ROOT_FOLDER                 "D:\\c\\exc\\source"
#define MAX_FILE_SIZE               0x10000
#define DEFAULT_FILES_PER_THREAD    1
#define DEFAULT_MODULES_PER_THREAD  1
#define DEFAULT_SYMBOLS_PER_THREAD  0x10
#define MAX_TYPER_SCOPE_DEPTH       0x100

#define MAX_NAME_FOR_HIGHLIGHT     0xC
#define DOTS                       2

namespace exy {
String binaryKind2Text(BinaryKind binaryKind) {
    switch (binaryKind) {
    #define ZM(zName, zText) case BinaryKind::zName: return { S(zText) };
        DeclareBinaryKinds(ZM)
    #undef ZM
    default: Unreachable();
    }
}
//------------------------------------------------------------------------------------------------
SourceLocation& SourceLocation::operator=(const SourceLocation &other)  {
    struct Dummy { SourceFile *file; SourceRange range; };
    auto a = (Dummy*)this;
    auto b = (Dummy*)&other;
    a->file = b->file;
    a->range = b->range;
    return *this;
}
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
        return loc.range.value();
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
String SourceToken::name(Tok t) {
    switch (t) {
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
String SourceToken::name(Keyword k) {
    switch (k) {
    #define ZM(zName, zText) case Keyword::zName: return { S(#zName), 0u };
        DeclareKeywords(ZM)
        DeclareModifiers(ZM)
        DeclareUserDefinedTypeKeywords(ZM)
        DeclareBuiltinTypeKeywords(ZM)
    #undef ZM
        default:
            Assert(0);
            break;
    }
    return {};
}
String SourceToken::value(Tok t) {
    if (t >= Tok::Text) {
        return { S("TEXT"), 0u };
    } switch (t) {
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
String SourceToken::value(Keyword k) {
    switch (k) {
    #define ZM(zName, zText) case Keyword::zName: return { S(zText), 0u };
        DeclareKeywords(ZM)
        DeclareModifiers(ZM)
        DeclareUserDefinedTypeKeywords(ZM)
    #undef ZM
    #define ZM(zName, zSize) case Keyword::zName: return { S(#zName), 0u };
        DeclareBuiltinTypeKeywords(ZM)
    #undef ZM
        default:
            Assert(0);
            break;
    }
    return {};
}
//------------------------------------------------------------------------------------------------
void Compiler::error(const char *pass,
                     const char *cppFile, const char *cppFunc, int cppLine,
                     const char *fmt, ...) {
    va_list ap{};
    __crt_va_start(ap, fmt);
    highlight(HighlightKind::Error, pass, cppFile, cppFunc, cppLine, fmt, ap);
    __crt_va_end(ap);
}

void Compiler::error(const char *pass, const SourceToken *pos,
                     const char *cppFile, const char *cppFunc, int cppLine,
                     const char *fmt, ...) {
    va_list ap{};
    __crt_va_start(ap, fmt);
    if (pos) {
        highlight(HighlightKind::Error, pass, pos->loc, cppFile, cppFunc, cppLine, fmt, ap);
    }
    __crt_va_end(ap);
}

void Compiler::error(const char *pass, const SourceToken &pos,
                     const char *cppFile, const char *cppFunc, int cppLine,
                     const char *fmt, ...) {
    va_list ap{};
    __crt_va_start(ap, fmt);
    highlight(HighlightKind::Error, pass, pos.loc, cppFile, cppFunc, cppLine, fmt, ap);
    __crt_va_end(ap);
}

void Compiler::error(const char *pass, const SourceLocation &loc, 
                     const char *cppFile, const char *cppFunc, int cppLine, 
                     const char *fmt, ...) {
    va_list ap{};
    __crt_va_start(ap, fmt);
    highlight(HighlightKind::Error, pass, loc, cppFile, cppFunc, cppLine, fmt, ap);
    __crt_va_end(ap);
}

void Compiler::error(const char *pass, const SyntaxNode *node,
                     const char *cppFile, const char *cppFunc, int cppLine,
                     const char *fmt, ...) {
    va_list ap{};
    __crt_va_start(ap, fmt);
    if (node) {
        auto &startPos = node->pos;
        auto   &endPos = node->lastpos();
        SourceLocation loc{ startPos.loc.file, startPos.loc.range.start, endPos.loc.range.end };
        highlight(HighlightKind::Error, pass, loc, cppFile, cppFunc, cppLine, fmt, ap);
    }
    __crt_va_end(ap);
}

void Compiler::error(const char *pass, const AstNode *node,
                     const char *cppFile, const char *cppFunc, int cppLine,
                     const char *fmt, ...) {
    va_list ap{};
    __crt_va_start(ap, fmt);
    if (node) {
        highlight(HighlightKind::Error, pass, node->loc, cppFile, cppFunc, cppLine, fmt, ap);
    }
    __crt_va_end(ap);
}

void Compiler::error(const char *pass, const IrNode *node,
                     const char *cppFile, const char *cppFunc, int cppLine,
                     const char *fmt, ...) {
    va_list ap{};
    __crt_va_start(ap, fmt);
    if (node) {
        highlight(HighlightKind::Error, pass, node->loc, cppFile, cppFunc, cppLine, fmt, ap);
    }
    __crt_va_end(ap);
}

//------------------------------------------------------------------------------------------------
void Compiler::compile() {
    options.path.sourceFolder = {};
    options.path.sourceFolder.append(ROOT_FOLDER);
    options.path.outputFolder = {};
    options.path.outputFolder.append(ROOT_FOLDER).append(S("\\.bin"));
    options.path.dumpFolder = {};
    options.path.dumpFolder.append(ROOT_FOLDER).append(S("\\.dump"));
    options.path.dump.ast = {};
    options.path.dump.ast.append(ROOT_FOLDER).append(S("\\.ast"));
    options.path.dump.ir = {};
    options.path.dump.ir.append(ROOT_FOLDER).append(S("\\.ir"));
    options.path.dump.ssa = {};
    options.path.dump.ssa.append(ROOT_FOLDER).append(S("\\.ssa"));
    options.path.dump.regalloc = {};
    options.path.dump.regalloc.append(ROOT_FOLDER).append(S("\\.regalloc"));
    options.maxFileSize             = MAX_FILE_SIZE;
    options.defaultFilesPerThread   = DEFAULT_FILES_PER_THREAD;
    options.defaultModulesPerThread = DEFAULT_MODULES_PER_THREAD;
    options.defaultSymbolsPerThread = DEFAULT_SYMBOLS_PER_THREAD;

    options.typer.maxScopeDepth  = MAX_TYPER_SCOPE_DEPTH;

    // Ensure that folders exist.
    if (!options.path.outputFolder.ensureFolderExists()) {
        traceln("cannot create or open %s#<yellow|underline>", &options.path.outputFolder);
        ++comp.errors;
    } if (!comp.errors) {
        if (src_pass::run()) {
            if (src2tok_pass::run()) {
                if (syn_pass::run()) {
                    if (stx2ast_pass::run()) {
                        if (ast2ir_pass::run()) {
                            if (opt_pass::run()) {
                                if (regalloc_pass::run()) {
                                    if (codegen_pass::run()) {
                                        if (linker_pass::run()) {
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ir  = MemDispose(ir);
    ast = MemDispose(ast);
    syntax = MemDispose(syntax);
    source = MemDispose(source);

    options.path.sourceFolder.dispose();
    options.path.outputFolder.dispose();
    options.path.dumpFolder.dispose();
    options.path.dump.ast.dispose();
    options.path.dump.ir.dispose();
    options.path.dump.ssa.dispose();
    options.path.dump.regalloc.dispose();
}

void Compiler::runBinaries() {

}

void Compiler::highlight(HighlightKind kind, const char *pass,
                         const char *cppFile, const char *cppFunc, int cppLine,
                         const char *fmt, va_list vargs) {
    auto stream = getConsoleFormatStream();
    stream.lock();
    printMessage(kind, pass, fmt, vargs);
    printCppLocation(cppFile, cppFunc, cppLine);
    stream.unlock();
}

void Compiler::printMessage(HighlightKind kind, const char *pass, const char *fmt, va_list vargs) {
    switch (kind) {
    case HighlightKind::Error: {
        trace("%c#<red>%cl#<red>: ", pass, S("Error"));
        ++errors;
    } break;
    case HighlightKind::Warning: {
        trace("%c#<yellow>%cl#<yellow>: ", pass, S("Warning"));
        ++warnings;
    } break;
    case HighlightKind::Info: {
        trace("%c#<green>%cl#<green>: ", pass, S("Info"));
        ++infos;
    } break;
    default: Assert(0);  break;
    }
    vprintln(nullptr, fmt, vargs);
}

void Compiler::highlight(HighlightKind kind, const char *pass, const SourceLocation &loc,
                         const char *cppFile, const char *cppFunc, int cppLine, 
                         const char *fmt, va_list vargs) {
    Assert(loc.range.start <= loc.range.end);
    auto stream = getConsoleFormatStream();
    stream.lock();
    printMessage(kind, pass, loc, fmt, vargs);
    printLines(kind, loc);
    printCppLocation(cppFile, cppFunc, cppLine);
    stream.unlock();
}

void Compiler::printMessage(HighlightKind kind, const char *pass, const SourceLocation &loc,
                            const char *fmt, va_list vargs) {
    auto  &file = loc.file;
    auto &range = loc.range;
    auto &start = range.start;
    auto   &end = range.end;
    switch (kind) {
        case HighlightKind::Error: {
            trace("%c#<red>%cl#<red> @ %s#<underline red>(%i#<darkred>:", pass, S("Error"),
                  file.dotName, start.line);
            if (start.line == end.line) {
                trace("%i#<darkred>—%i#<darkred>)  →  ", start.col, end.col);
            } else {
                trace("%i#<darkred>—%i#<darkred>:%i#<darkred>)  →  ", start.col, end.line, end.col);
            }
            ++errors;
        } break;
        case HighlightKind::Warning: {
            trace("%c#<yellow>%cl#<yellow> @ %s#<underline yellow>(%i#<darkyellow>:", pass, S("Warning"), 
                  file.dotName, start.line);
            if (start.line == end.line) {
                trace("%i#<darkyellow>—%i#<darkyellow>)  →  ", start.col, end.col);
            } else {
                trace("%i#<darkyellow>—%i#<darkyellow>:%i#<darkyellow>)  →  ", start.col, end.line, end.col);
            }
            ++warnings;
        } break;
        case HighlightKind::Info: {
            trace("%c#<green>%cl#<green> @ %s#<underline green>(%i#<darkgreen>:", pass, S("Info"),
                  file.dotName, start.line);
            if (start.line == end.line) {
                trace("%i#<darkgreen>—%i#<darkgreen>)  →  ", start.col, end.col);
            } else {
                trace("%i#<darkgreen>—%i#<darkgreen>:%i#<darkgreen>)  →  ", start.col, end.line, end.col);
            }
            ++infos;
        } break;
        default: Assert(0);  break;
    }
    vprintln(nullptr, fmt, vargs);
}

void Compiler::printCppLocation(const char *cppFile, const char *cppFunc, int cppLine) {
    traceln("\t%cl#<bold> %c#<magenta> @ %c#<underline cyan>:%i#<darkcyan>", S("from"), cppFunc, cppFile, cppLine);
}

static const char* findLineStart(const char *start, const char *srcstart) {
    Assert(*start != '\n');
    Assert(start > srcstart);
    for (--start; start > srcstart && *start != '\n'; --start);
    if (*start == '\n') {
        ++start;
    }
    return start;
}

static const char* findLineEnd(const char *end, const char *srcend) {
    Assert(*end != '\n');
    for (++end; end < srcend && *end != '\n'; ++end);
    if (end < srcend && *end == '\n') {
        if (end[-1] == '\r') {
            --end;
        }
    }
    return end;
}

static Compiler::Line makeLineFrom(const String &source, const char *pos, int line) {
    auto srcstart = source.text;
    auto   srcend = source.end();
    auto    start = pos;
    auto      end = pos;
    if (*start == '\n') {
        if (start > srcstart && start[-1] == '\r') {
            --start;
            --end;
        }
    } else if (start == srcstart) {
        if (end == srcend) { // SOF and EOF.
            // Do nothing.
        } else { // SOF but not EOF.
            end = findLineEnd(end, srcend);
        }
    } else if (start == srcend) { // EOF but not SOF.
        start = findLineStart(start, srcstart);
    } else {
        start = findLineStart(start, srcstart);
        end = findLineEnd(end, srcend);
    }
    return { start, end, line };
}

static Compiler::Line makePreviousLine(const String &source, const char *pos, int line) {
    auto srcstart = source.text;
    auto    start = pos;
    auto      end = pos;
    Assert(start >= srcstart);
    if (start > srcstart) {
        end = --start;
        Assert(*end == '\n');
        --line;
        Assert(line > 0);
        if (start == srcstart) {
            // Do nothing.
        } else if (*start == '\n') {
            end = --start;
            start = findLineStart(start, srcstart);
        } else {
            start = findLineStart(start, srcstart);
        }
    } else {
        line = 0;
    }
    return { start, end, line };
}

static Compiler::Line makeNextLine(const String &source, const char *pos, int line) {
    auto srcend = source.end();
    auto   start = pos;
    auto     end = pos;
    if (end < srcend) {
        start = ++end;
        ++line;
        if (end == srcend) {
            // Do nothing.
        } else if (*end == '\n') {
            start = ++end;
            end = findLineEnd(end, srcend);
        } else {
            end = findLineEnd(end, srcend);
        }
    } else {
        line = 0;
    }
    return { start, end, line };
}

void Compiler::printLines(HighlightKind kind, const SourceLocation &loc) {
    Line lines[maxLines]{};
    MemZero(lines, maxLines);
    auto   &src = loc.file.source;
    auto &range = loc.range;
    collectLines(src, range, lines);
    highlightLines(kind, range, lines);
}

void Compiler::collectLines(const String &src, const SourceRange &range, Line *lines) {
    auto line = makeLineFrom(src, range.start.pos, range.start.line);
    lines[maxLinesAbove] = line;
    for (auto i = maxLinesAbove; i > 0; --i) {
        line = makePreviousLine(src, lines[i].start, lines[i].number);
        if (!line.number) {
            break;
        }
        Assert(line.start <= line.end);
        lines[i - 1] = line;
    }
    for (auto i = maxLinesAbove; i < maxLines - 1; ++i) {
        line = makeNextLine(src, lines[i].end, lines[i].number);
        if (!line.number) {
            break;
        }
        Assert(line.start <= line.end);
        lines[i + 1] = line;
    }
}

void Compiler::highlightLines(HighlightKind kind, const SourceRange &hotRange, Line *lines) {
    for (auto i = 0; i < maxLines; ++i) {
        auto &line = lines[i];
        if (!line.number) {
            continue;
        }
        highlightLine(kind, hotRange, line);
    }

}

static const int maxHotSpots = 3;
struct HotSpot {
    const char *start;
    const char *end;
    bool        isHot;
};

static void makeHotSpots(const char *hotStart, const char *hotEnd,
                         const char *lineStart, const char *lineEnd,
                         HotSpot *hot) {
    if (hotEnd < lineStart || hotStart > lineEnd) { // Not hot.
        hot[0] = { lineStart, lineEnd, false };
        return;
    }
    hot[0] = { lineStart,  max(hotStart, lineStart), false };
    hot[1] = { hot[0].end, min(hotEnd, lineEnd),     true };
    hot[2] = { hot[1].end, lineEnd,                  false };
}

void Compiler::highlightLine(HighlightKind kind, const SourceRange &hotRange, const Line &line) {
    // Print line number.
    _itoa_s(line.number, numbuf, 10);
    auto length = cstrlen(numbuf);
    auto spaces = lineNumberSize - length;
    if (spaces > 0) {
        for (auto i = 0; i < spaces; ++i) trace(" ");
    } switch (kind) {
        case HighlightKind::Error:   trace("%cl#<darkred>",    numbuf, length); break;
        case HighlightKind::Warning: trace("%cl#<darkyellow>", numbuf, length); break;
        case HighlightKind::Info:    trace("%cl#<darkgreen>",  numbuf, length); break;
        default: Assert(0);  break;
    }

    if (line.number < hotRange.start.line || line.number > hotRange.end.line) {
        trace("%cl#<darkred>", S(" |  "));
        traceln("%cl#<bold>", line.start, (int)(line.end - line.start));
    } else {
        trace("%cl#<red>", S(" →  "));
        HotSpot hotSpots[maxHotSpots]{};
        MemZero(hotSpots, maxHotSpots);
        makeHotSpots(hotRange.start.pos, hotRange.end.pos, line.start, line.end, hotSpots);
        for (auto i = 0; i < maxHotSpots; ++i) {
            auto &spot = hotSpots[i];
            length = (int)(spot.end - spot.start);
            Assert(length >= 0);
            if (length == 0) {
                if (i) {
                    trace("%cl#<red>", S("←"));
                } else {
                    trace("%cl#<red>", S("→"));
                }
            } else if (spot.isHot) {
                trace("%cl#<underline red>", spot.start, length);
            } else {
                trace("%cl#<bold>", spot.start, length);
            }
        }
        traceln("");
    }

}

int signedSize(INT64 n) {
    union Size {
        struct {
            INT32 lo;
            INT32 hi;
        } bits32;
        struct {
            INT16 lo16lo32;
            INT16 hi16lo32;
            INT16 lo16hi32;
            INT16 hi16hi32;
        } bits16;
        struct {
            INT8 lo8lo16lo32;
            INT8 hi8lo16lo32;

            INT8 lo8hi16lo32;
            INT8 hi8hi16lo32;

            INT8 lo8lo16hi32;
            INT8 hi8lo16hi32;

            INT8 lo8hi16hi32;
            INT8 hi8hi16hi32;
        } bits8;
        INT64 i64;
    };
    Size sz;
    sz.i64 = n;
    if (sz.bits32.hi == 0i32) {
        if (sz.bits16.hi16lo32 == 0i16) {
            if (sz.bits8.hi8lo16lo32 == 0i8) {
                return sizeof(__int8);
            }
            return sizeof(__int16);
        }
        return sizeof(int);
    } if (sz.bits32.hi == -1i32) {
        if (sz.bits16.hi16lo32 == -1i16) {
            if (sz.bits8.hi8lo16lo32 == -1i8) {
                return sizeof(__int8);
            }
            return sizeof(__int16);
        }
        return sizeof(__int32);
    }
    return sizeof(__int64);
}

int unsignedSize(UINT64 n) {
    union Size {
        struct {
            UINT32 lo;
            UINT32 hi;
        } bits32;
        struct {
            UINT16 lo16lo32;
            UINT16 hi16lo32;
            UINT16 lo16hi32;
            UINT16 hi16hi32;
        } bits16;
        struct {
            UINT8 lo8lo16lo32;
            UINT8 hi8lo16lo32;
            UINT8 lo8hi16lo32;
            UINT8 hi8hi16lo32;
            UINT8 lo8lo16hi32;
            UINT8 hi8lo16hi32;
            UINT8 lo8hi16hi32;
            UINT8 hi8hi16hi32;
        } bits8;
        UINT64 u64;
    };
    Size sz;
    sz.u64 = n;
    if (sz.bits32.hi == 0i32) {
        if (sz.bits16.hi16lo32 == 0i16) {
            if (sz.bits8.hi8lo16lo32 == 0i8) {
                return sizeof(__int8);
            }
            return sizeof(__int16);
        }
        return sizeof(__int32);
    }
    return sizeof(__int64);
}

namespace compiler {
void run(int compId) {
    traceln("%cl#<underline yellow> is the %cl#<underline red> language compiler", 
            S("exc"), S("exy"));
    traceln("Compid: %i#<magenta>", compId);
    traceln("Thread: %u#<magenta>", GetCurrentThreadId());

    ids = Identifiers{};
    ids.initialize();

    comp = Compiler{};
    comp.compile();
    comp.runBinaries();

    ids.dispose();
}
} // namespace compiler
} // namespace exy
