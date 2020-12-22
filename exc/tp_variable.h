//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_VARIABLE_H_
#define TP_VARIABLE_H_

namespace exy {
namespace typ_pass {
struct Variable {
    using  Decl = SyntaxNameValue*;
    using  Name = SyntaxIdentifier*;
    using Tuple = SyntaxCommaList*;
    using  Mods = SyntaxNode*;
    using  Type = const AstType&;
    using  Node = AstNode*;

    Typer &tp;

    Variable(Typer *tp) : tp(*tp) {}

    Node visit(Decl);
private:
    Node visit(Decl, Name);
    Node visit(Decl, Tuple);

    Node make(Loc, Mods, Name, Node);
    Node make(Loc, Mods, Name, Type);
    Node make(Loc, Mods, Name, Type, Node);

    Node make(Loc, Mods, Tuple, Node);

    AstKind getKind(Mods modifiers);
};
} // namespace typ_pass
} // namespace exy

#endif // TP_VARIABLE_H_