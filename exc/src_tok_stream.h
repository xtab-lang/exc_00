//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-09
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SRC_TOK_STREAM_H_
#define SRC_TOK_STREAM_H_

namespace exy {
//--Begin forward declarations
enum class Tok;
struct SourceChar;
struct SourceFile;
struct SourceToken;
//----End forward declarations
struct TokenStream {
    const SourceFile &file;
    const SourceChar *mark;
    const SourceChar *cur;
    const SourceChar *end;

    TokenStream(const SourceFile &file, const List<SourceChar> &list);

    SourceToken next();
    SourceToken skipZeroLength();
    Tok read();
    Tok skipWhiteSpace();
    Tok readNumber();
    Tok tryDecimal();
    Tok tryFloat();
    Tok tryExponent(const SourceChar *dot);

    Tok tryHexadecimal();
    Tok trySuffixedHexadecimal();

    Tok tryBinary();
    Tok trySuffixedBinary();

    Tok tryOctal();
    Tok trySuffixedOctal();

    Tok readSuffix(Tok);
    Tok readText();
    Tok move(Tok);
    SourceToken make(Tok);
}; // struct TokenStream
} // namespace exy

#endif // SRC_TOK_STREAM_H_