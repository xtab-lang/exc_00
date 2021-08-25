#include "pch.h"

#include "src.h"
#include "typer.h"

namespace exy {
struct Line {
    using Pos = const CHAR*;

    Pos start;
    Pos end;
    INT line;

    Line previousLine(Pos srcStart) const {
        Line ln{ nullptr, nullptr, line - 1 };
        if (ln.line == 0) {
            return ln;
        }
        ln.end = start - 1;
        Assert(ln.end >= srcStart && *ln.end == '\n');
        if (ln.end > srcStart && ln.end[-1] == '\r') {
            --ln.end;
        }
        ln.start = ln.end;
        if (ln.start > srcStart) {
            for (--ln.start; ln.start > srcStart; --ln.start) {
                if (*ln.start == '\n') {
                    ++ln.start;
                    break;
                }
            }
        }
        return ln;
    }

    Line nextLine(Pos srcEnd, INT maxLines) const {
        Line ln{ nullptr, nullptr, line + 1 };
        if (ln.line > maxLines) {
            ln.line = 0;
            return ln;
        }
        if (*end == '\r' && end[1] == '\n') {
            ln.start = end + 2;
        } else if (*end == '\n') {
            ln.start = end + 1;
        } else {
            Assert(0);
        }
        Assert(ln.start <= srcEnd);
        ln.end = ln.start;
        if (ln.end < srcEnd && *ln.end != '\n') {
            for (++ln.end; ln.end < srcEnd; ++ln.end) {
                if (*ln.end == '\n') {
                    if (ln.end[-1] == '\r') {
                        --ln.end;
                    }
                    break;
                }
            }
        }
        return ln;
    }

    auto isEmpty() const {
        return start == end;
    }

    auto isNewLine() const {
        auto length = INT(end - start);
        if (length == 2 && *start == '\r' && end[1] == '\n') {
            return true;
        }
        if (length == 1 && *start == '\n') {
            return true;
        }
        return false;
    }
};

static auto makeLine(const CHAR *srcStart, const CHAR *hotPos, INT line, const CHAR *srcEnd) {
    Line ln{ hotPos, hotPos, line };
    for (; ln.start > srcStart; --ln.start) {
        if (*ln.start == '\n') {
            break;
        }
    }
    if (ln.start > srcStart && ln.start < hotPos) {
        ++ln.start;
    }
    for (; ln.end < srcEnd; ++ln.end) {
        if (*ln.end == '\n') {
            break;
        }
    }
    if (ln.end < srcEnd && ln.end > hotPos) {
        if (ln.end - 1 >= srcStart && ln.end[-1] == '\r') {
            --ln.end;
        }
    }
    return ln;
}
constexpr auto maxLinesBefore = 3;
constexpr  auto maxLinesAfter = 2;
static thread_local Line lines[maxLinesBefore + 1 + maxLinesAfter]{};

static auto makeLines(const CHAR *srcStart, const Line &hotLine, const CHAR *srcEnd, INT maxLines) {
    Line lns[maxLinesBefore]{};
    auto length = 0;
    for (auto i = 0; i < maxLinesBefore; ++i) {
        auto ln = length > 0 ? lns[length - 1].previousLine(srcStart) : hotLine.previousLine(srcStart);
        if (ln.line == 0) {
            break;
        }
        lns[length++] = ln;
    }
    auto j = 0;
    for (auto i = length; --i >= 0; ) {
        lines[j++] = lns[i];
    }
    lines[j++] = hotLine;
    for (auto i = 0; i < maxLinesAfter; i++) {
        auto ln = lines[j - 1].nextLine(srcEnd, maxLines);
        if (ln.line == 0) {
            break;
        }
        lines[j++] = ln;
    }
    return j;
}

struct HotLine {
    const CHAR *lnStart;
    const CHAR *start;
    const CHAR *end;
    const CHAR *lnEnd;
    bool hasEllipsis;
};

static auto parseHotLine(const Line &ln, const CHAR *hotStart, const CHAR *hotEnd) {
    HotLine hot{ ln.start, hotStart, hotEnd, ln.end, false };
    if (hot.start < hot.lnStart) {
        hot.start = hot.lnStart;
    }
    if (hot.end > hot.lnEnd) {
        hot.end = hot.lnEnd;
        hot.hasEllipsis = true;
    }
    return hot;
}

INT Compiler::highlight(const SourceFile *file, const SourceChar *hotStart, const SourceChar *hotEnd) {
    const auto     &src = file->source;
    const auto srcStart = src.start();
    const auto   srcEnd = src.end();
    Assert(hotStart->text <= hotEnd->text);
    Assert(hotStart->text >= srcStart && hotStart->text <= srcEnd);
    Assert(hotEnd->text >= srcStart && hotEnd->text <= srcEnd);
    Assert(hotStart->line <= hotEnd->line);
    if (hotStart->line == hotEnd->line) {
        Assert(hotStart->col <= hotEnd->col);
    }
    const auto hotLine = makeLine(srcStart, hotStart->text, hotStart->line, srcEnd);
    const auto     lns = makeLines(srcStart, hotLine, srcEnd, file->lines);
    Assert(lns > 0);

    auto maxLineNumberLength = 0;
    auto       maxLineLength = 0;
    for (auto i = 0; i < lns; i++) {
        const auto &ln = lines[i];
        Assert(ln.start <= ln.end);
        _itoa_s(ln.line, tmpbuf, 10);
        auto lineNumberLength = cstrlen(tmpbuf);
        if (lineNumberLength > maxLineNumberLength) {
            maxLineNumberLength = lineNumberLength;
        }
        auto lineLength = INT(ln.end - ln.start);
        if (lineLength > maxLineLength) {
            maxLineLength = lineLength;
        }
    }
    for (auto i = 0; i < lns; i++) {
        const auto &ln = lines[i];
        _itoa_s(ln.line, tmpbuf, 10);
        auto lineNumberLength = cstrlen(tmpbuf);
        for (auto j = lineNumberLength; j < maxLineNumberLength; j++) {
            trace(" ");
        }
        if (ln.line == hotLine.line) {
            trace("%i#<darkred>  %c#<red>", ln.line, "→");
        } else if (i + 1 == lns) {
            trace("%i#<darkgreen>  %c#<darkyellow underline>", ln.line, "v");
        } else {
            trace("%i#<darkgreen>  %c#<green>", ln.line, "|");
        }
        if (ln.isNewLine() || ln.isEmpty()) {
            // Do nothing.
        } else if (ln.line == hotLine.line) {
            auto hot = parseHotLine(ln, hotStart->text, hotEnd->text);
            String str{ hot.lnStart, hot.start };
            trace("  %s", &str);
            str = { hot.start, hot.end };
            if (str.isEmpty()) {
                trace("%c#<darkred>", "«EOF»");
            } else {
                trace("%s#<red underline>", &str);
            }
            str = { hot.end, hot.lnEnd };
            if (str.isEmpty()) {
                if (hot.hasEllipsis) {
                    trace("…");
                }
            } else {
                trace("%s", &str);
            }
        } else {
            String str{ ln.start, ln.end };
            trace("  %s", &str);
        }
        traceln("");
    }
    return maxLineNumberLength;
}

constexpr auto maxGraphFileNameLength = 0x10;
constexpr auto maxGraphLocationLength = 0x14;

static auto int2strLength(auto n) {
    _itoa_s(n, tmpbuf, 10);
    return cstrlen(tmpbuf);
}

void Compiler::graph(INT indent) {
    if (typer == nullptr) {
        return;
    }
    auto &tp = *typer;
    if (tp.current == nullptr || tp.current->site == nullptr) {
        return;
    }
    for (auto i = 0; i < indent; i++) {
        trace(" ");
    }
    traceln("  %c#<darkyellow>", "Call graph");
    for (auto site = tp.current->site; site != nullptr; site = site->prev) {
        if (site->pos == nullptr) {
            continue;
        }
        const auto       pos = tp.mkPos(site->pos);
        const auto     &file = pos.file; 
        const auto      &src = file.source;
        const auto  srcStart = src.start();
        const auto    srcEnd = src.end();
        const auto &hotStart = pos.range.start;
        const auto   &hotEnd = pos.range.end;
        Assert(hotStart.text <= hotEnd.text);
        Assert(hotStart.text >= srcStart && hotStart.text <= srcEnd);
        Assert(hotEnd.text >= srcStart && hotEnd.text <= srcEnd);
        Assert(hotStart.line <= hotEnd.line);
        if (hotStart.line == hotEnd.line) {
            Assert(hotStart.col <= hotEnd.col);
        }
        const auto line = makeLine(srcStart, hotStart.text, hotStart.line, srcEnd);
        // fileName(line:col) source
        const auto fileName = file.dotName;
        if (fileName->length > maxGraphFileNameLength) {
            const auto diff = fileName->length - maxGraphFileNameLength;
            String shortName{ fileName->text + diff + 3, fileName->end() };
            trace("...");
            trace("%s#<yellow>", &shortName);
        } else {
            for (auto i = fileName->length; i < maxGraphFileNameLength; i++) {
                trace(" ");
            }
            trace("%s#<yellow>", fileName);
        }
        trace("(");
        auto locationLength = 1;
        if (hotStart.line == hotEnd.line) {
            if (hotStart.col == hotEnd.col) {
                locationLength += int2strLength(hotStart.line) + 1 + int2strLength(hotStart.col) + 
                    int2strLength(hotEnd.col) + 1;
                trace("%i#<yellow>:%i#<yellow>", hotStart.line, hotStart.col);
            } else {
                locationLength += int2strLength(hotStart.line) + 1 + int2strLength(hotStart.col) + 1 +
                    int2strLength(hotEnd.col);
                trace("%i#<yellow>:%i#<yellow>―%i#<yellow>", hotStart.line, hotStart.col, hotEnd.col);
            }
        } else {
            locationLength += int2strLength(hotStart.line) + 1 + int2strLength(hotStart.col) + 1 +
                int2strLength(hotEnd.line) + 1 + int2strLength(hotEnd.col);
            trace("%i#<yellow>:%i#<yellow>―%i#<yellow>:%i#<yellow>", hotStart.line, hotStart.col,
                  hotEnd.line, hotEnd.col);
        }
        ++locationLength;
        trace(")");
        for (; locationLength < maxGraphLocationLength; ++locationLength) {
            trace(" ");
        }
        trace("%c#<darkyellow>", "|");
        auto hot = parseHotLine(line, hotStart.text, hotEnd.text);
        String str{ hot.lnStart, hot.start };
        trace("  %s", &str);
        str = { hot.start, hot.end };
        if (str.isEmpty()) {
            trace("%c#<darkred>", "«EOF»");
        } else {
            trace("%s#<red underline>", &str);
        }
        str = { hot.end, hot.lnEnd };
        if (str.isEmpty()) {
            if (hot.hasEllipsis) {
                trace("…");
            }
        } else {
            trace("%s", &str);
        }
        traceln("");
    }
}
} // namespace exy