//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef STX2AST_CAST_H_
#define STX2AST_CAST_H_

namespace exy {
namespace stx2ast_pass {
struct Cast {
    Typer &tp;

    Cast(Typer &tp) : tp(tp) {}

    AstNode* explicitCast(Loc, AstNode*, const AstType&);
    AstNode* implicitCast(Loc, AstNode*, const AstType&);
private:
};
} // namespace stx2ast_pass
} // namespace exy

#endif // STX2AST_CAST_H_