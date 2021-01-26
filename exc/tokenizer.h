//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-27
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef LEXER_H_
#define LEXER_H_

namespace exy {
namespace src2tok_pass {
struct Tokenizer {
    void next(SourceFile &file);
    void tokenize(SourceFile &file, const List<SourceChar> &chars);
};
} // namespace src2tok_pass
} // namespace exy

#endif // LEXER_H_