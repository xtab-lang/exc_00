//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-05
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SYN_CURSOR_H
#define SYN_CURSOR_H

namespace exy {
//--Begin forward declarations
struct SourceToken;

using Token = const SourceToken*;
//----End forward declarations
struct Cursor {
    Token prev;
    Token pos;
    Token end;

    Cursor(Token pos, Token end);

    Token advance();
    Token peek();

    bool skipWhiteSpace();
    void skipSingleLineComment();
    void skipMultiLineComment();
};
} // namespace exy

#endif // SYN_CURSOR_H