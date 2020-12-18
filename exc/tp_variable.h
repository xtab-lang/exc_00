//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_VARIABLES_H_
#define TP_VARIABLES_H_

namespace exy {
namespace typ_pass {
//--Begin forward declarations
struct Typer;
//----End forward declarations
struct Variable {
    using Decl = SyntaxNameValue*;
    using Name = SyntaxIdentifier*;
    using Mods = SyntaxNode*;

    Typer &tp;

    Variable(Typer *tp) : tp(*tp) {}

    AstNode* visit(Decl);
private:
    AstNode* visit(Decl, Name);

    AstNode* make(Mods, Name, AstNode*);

    AstKind getKind(Mods modifiers);
};
} // namespace typ_pass
} // namespace exy

#endif // TP_VARIABLES_H_