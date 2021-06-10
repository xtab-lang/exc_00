#pragma once

namespace exy {
struct SourceFile;
//----------------------------------------------------------
struct SourceChar {
    const CHAR *pos;
    INT         line;
    INT         col;
};
//----------------------------------------------------------
struct SourceRange {
    SourceChar start;
    SourceChar end;
};
//----------------------------------------------------------
struct SourcePos {
    const SourceFile &file;
    SourceRange       range;
};
//----------------------------------------------------------
struct SourceToken {
    SourcePos pos;
    Tok       kind;
    Keyword   keyword;
};
} // namespace exy