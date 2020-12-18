//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-27
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef LEXER_H_
#define LEXER_H_

namespace exy {
//--Begin forward declarations
struct SourceChar;
struct SourceFile;
//----End forward declarations
struct Tokenizer {
    void next(SourceFile &file);
    void tokenize(SourceFile &file, const List<SourceChar> &chars);
};
} // namespace exy

#endif // LEXER_H_