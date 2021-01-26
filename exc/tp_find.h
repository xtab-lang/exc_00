//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-20
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef STX2AST_FIND_H_
#define STX2AST_FIND_H_

namespace exy {
namespace stx2ast_pass {
struct Find {
    Typer &tp;

    Find(Typer *tp) : tp(*tp) {}

    AstNode* name(SyntaxIdentifier*);
    AstNode* name(Loc, Identifier);
};
} // namespace stx2ast_pass
} // namespace exy

#endif // STX2AST_FIND_H_