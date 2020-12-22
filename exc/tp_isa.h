//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_ISA_H_
#define TP_ISA_H_

namespace exy {
namespace typ_pass {
struct Isa {
    Typer &tp;

    Isa(Typer *tp) : tp(*tp) {}

    AstAlias* alias(AstNode*);

#define ZM(zName) Ast##zName* zName(AstNode *node) { return node ? node->kind == AstKind::zName ? (Ast##zName*)node : nullptr : nullptr; }
    DeclareAstNodes(ZM)
#undef ZM

#define ZM(zName) Ast##zName* zName(const AstType &type) { return zName(type.isaSymbol()); }
    DeclareAstNodes(ZM)
#undef ZM
        
#define ZM(zName, zSize) AstBuiltin* zName(const AstType &type) { if (auto b = Builtin(type)) return b->keyword == Keyword::zName ? b : nullptr; return nullptr; }
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
};
} // namespace typ_pass
} // namespace exy

#endif // TP_ISA_H_