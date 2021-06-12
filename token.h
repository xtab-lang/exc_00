#pragma once

namespace exy {
struct SourceFile;
//----------------------------------------------------------
struct SourceChar {
    const CHAR *text;
    INT         line;
    INT         col;

    SourceChar() = delete;
    SourceChar(const CHAR *text, INT line, INT col) : text(text), line(line), col(col) {}
    auto operator>(const SourceChar &other) const  { return line > other.line || (line == other.line && col > other.col);  }
    auto operator>=(const SourceChar &other) const { return line > other.line || (line == other.line && col >= other.col); }
    auto operator<(const SourceChar &other) const  { return line < other.line || (line == other.line && col < other.col);  }
    auto operator<=(const SourceChar &other) const { return line < other.line || (line == other.line && col <= other.col); }
};
//----------------------------------------------------------
struct SourceRange {
    SourceChar start;
    SourceChar end;

    SourceRange() = delete;
    SourceRange(const SourceRange&) = default;
    SourceRange(const SourceChar &start, const SourceChar &end) : start(start), end(end) {
        Assert(start <= end);
    }
    auto operator>(const SourceRange &other) const  { return start > other.end;  }
    auto operator>=(const SourceRange &other) const { return start >= other.end; }
    auto operator<(const SourceRange &other) const  { return end < other.start;  }
    auto operator<=(const SourceRange &other) const { return end <= other.start; }
    String value() const { return { start.text, (INT)(end.text - start.text) }; }
};
//----------------------------------------------------------
struct SourcePos {
    const SourceFile &file;
    SourceRange       range;

    SourcePos() = delete;
    SourcePos(const SourcePos&) = default;
    SourcePos(const SourceFile &file, const SourceChar &start, const SourceChar &end)
        : file(file), range(start, end) {}
    String sourceValue() const { return range.value(); }

    SourcePos& operator=(const SourcePos & other);
};
using Pos = const SourcePos&;
//----------------------------------------------------------
struct SourceToken {
    SourcePos pos;
    Tok       kind;
    Keyword   keyword;

    SourceToken() = delete;
    SourceToken(const SourceToken&) = default;
    SourceToken(const SourceFile &file, const SourceChar &start, const SourceChar &end, Tok kind) :
        pos(file, start, end), kind(kind), keyword(Keyword::None) {}
    String name() const;
    String value() const;
    static String name(Tok);
    static String name(Keyword);
    static String value(Tok);
    static String value(Keyword);
    String sourceValue() const { return pos.sourceValue(); }

    SourceToken& operator=(const SourceToken&) = default;
};
} // namespace exy