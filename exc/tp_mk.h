//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_MK_H_
#define TP_MK_H_

namespace exy {
namespace typ_pass {
struct Make {
    Typer &tp;

    Make(Typer *tp) : tp(*tp) {}

    AstAlias* importOf(Loc, Identifier name, AstSymbol*);
    AstAlias* exportOf(Loc, Identifier name, AstSymbol*);
    AstAlias* alias(Loc, AstAliasKind decl, Identifier name, AstSymbol *symbol);

    AstGlobal* global(Loc, Identifier name, const AstType &type);
    AstGlobal* global(Loc, Identifier name, AstNode* rhs);

    AstNode* name(Loc, AstSymbol*);
    AstTypeName* tpname(Loc, const AstType&);

    AstNode* explicitCast(Loc, AstNode *src, const AstType &dst);
    AstNode* implicitCast(Loc, AstNode *src, const AstType &dst);

};
} // namespace typ_pass
} // namespace exy

#endif // TP_MK_H_