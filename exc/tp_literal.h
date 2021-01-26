//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef STX2AST_LITERAL_H_
#define STX2AST_LITERAL_H_

namespace exy {
namespace stx2ast_pass {
struct Literal {
    Typer &tp;

    Literal(Typer *tp) : tp(*tp) {}

    AstConstant* visit(SyntaxLiteral*);
};
} // namespace stx2ast_pass
} // namespace exy

#endif // STX2AST_LITERAL_H_