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

void Compiler::error(const SourceToken *pos, const char *cppFile, const char *cppFunc, int cppLine,
                     const char *fmt, ...) {
    va_list ap{};
    __crt_va_start(ap, fmt);
    if (pos) {
        highlight(HighlightKind::Error, *pos, *pos, cppFile, cppFunc, cppLine, fmt, ap);
    }
    __crt_va_end(ap);
}

void Compiler::error(const SourceToken &pos, const char *cppFile, const char *cppFunc, int cppLine,
                     const char *fmt, ...) {
    va_list ap{};
    __crt_va_start(ap, fmt);
    highlight(HighlightKind::Error, pos, pos, cppFile, cppFunc, cppLine, fmt, ap);
    __crt_va_end(ap);
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

void Compiler::highlight(HighlightKind kind, const SourceToken &start, const SourceToken &end,
                         const char *cppFile, const char *cppFunc, int cppLine, 
                         const char *fmt, va_list vargs) {
    Assert(start.file);
    Assert(start.file == end.file);

    Assert(start.range.start.line > 0 && start.range.start.col > 0);
    Assert(start.range.end.line   > 0 && start.range.end.col   > 0);
    Assert(end.range.start.line   > 0 && end.range.start.col   > 0);
    Assert(end.range.end.line     > 0 && end.range.end.col     > 0);

    Assert(start.range.start.line <= start.range.end.line);
    if (start.range.start.line == start.range.end.line) {
        Assert(start.range.start.col <= start.range.end.col);
    }

    Assert(end.range.start.line <= end.range.end.line);
    if (end.range.start.line == end.range.end.line) {
        Assert(end.range.start.col <= end.range.end.col);
    }

    Assert(start.range.start.line <= end.range.start.line);
    if (start.range.start.line == end.range.start.line) {
        Assert(start.range.start.col <= end.range.start.col);
        if (start.range.start.col == end.range.start.col) {
            Assert(start.range.end.col == end.range.end.col);
        } else {
            Assert(start.range.end.col <= end.range.start.col);
        }
    }
    auto stream = getConsoleFormatStream();
    stream.lock();
    SourceRange range{ start.range.start, end.range.end };
    printMessage(kind, *start.file, range, fmt, vargs);
    printLines(kind, start.file->source, range);
    printCppLocation(cppFile, cppFunc, cppLine);
    stream.unlock();
}

void Compiler::printMessage(HighlightKind kind, const SourceFile &file, const SourceRange &range,
                            const char *fmt, va_list vargs) {
    auto &start = range.start;
    auto   &end = range.end;
    switch (kind) {
        case HighlightKind::Error: {
            trace("%cl#<red> @ %s#<underline red>(%i#<darkred>:", S("error"), file.dotName,
                  start.line);
            if (start.line == end.line) {
                trace("%i#<darkred>—%i#<darkred>)  →  ", start.col, end.col);
            } else {
                trace("%i#<darkred>—%i#<darkred>:%i#<darkred>)  →  ", start.col, end.line, end.col);
            }
            ++errors;
        } break;
        case HighlightKind::Warning: {
            trace("%cl#<yellow> @ %s#<underline yellow>(%i#<darkyellow>:", S("warning"), file.dotName,
                  start.line);
            if (start.line == end.line) {
                trace("%i#<darkyellow>—%i#<darkyellow>)  →  ", start.col, end.col);
            } else {
                trace("%i#<darkyellow>—%i#<darkyellow>:%i#<darkyellow>)  →  ", start.col, end.line, end.col);
            }
            ++warnings;
        } break;
        case HighlightKind::Info: {
            trace("%cl#<green> @ %s#<underline green>(%i#<darkgreen>:", S("info"), file.dotName,
                  start.line);
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
    traceln("\t%cl#<bold> %c#<magenta> @ %c#<underline cyan>(%i#<darkcyan>)", S("from"), cppFunc, cppFile, cppLine);
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
    if (start > srcstart) {
        end = --start;
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

void Compiler::printLines(HighlightKind kind, const String &src, const SourceRange &range) {
    Line lines[maxLines]{};
    memzero(lines, maxLines);
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
    hot[0] = { lineStart,  hotStart,             false };
    hot[1] = { hotStart,   min(hotEnd, lineEnd), true };
    hot[2] = { min(hotEnd, lineEnd), lineEnd,    false };
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
        memzero(hotSpots, maxHotSpots);
        makeHotSpots(hotRange.start.pos, hotRange.end.pos, line.start, line.end, hotSpots);
        for (auto i = 0; i < maxHotSpots; ++i) {
            auto &spot = hotSpots[i];
            length = (int)(spot.end - spot.start);
            Assert(length >= 0);
            if (length == 0) {
                // Do nothing.
            } else if (spot.isHot) {
                trace("%cl#<underline red>", spot.start, length);
            } else {
                trace("%cl#<bold>", spot.start, length);
            }
        }
        traceln("");
    }

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
}
// namespace comp_pass
} // namespace exy
