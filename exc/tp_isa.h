//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef STX2AST_ISA_H_
#define STX2AST_ISA_H_

namespace exy {
namespace stx2ast_pass {
struct Isa {
    using Node = AstNode*;
    using Type = const AstType&;

    AstPointerType*   PointerType(  const AstType&);
    AstReferenceType* ReferenceType(const AstType&);

#define ZM(zName) Ast##zName* zName(Node node) { return node ? node->kind == AstKind::zName ? (Ast##zName*)node : nullptr : nullptr; }
    DeclareAstNodes(ZM)
#undef ZM

#define ZM(zName) Ast##zName* zName(Type type) { return zName(type.isaSymbol()); }
    DeclareAstTypeSymbols(ZM)
#undef ZM
        
#define ZM(zName, zSize) AstBuiltin* zName(Type type) { if (auto b = Builtin(type)) return b->keyword == Keyword::zName ? b : nullptr; return nullptr; }
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
};
} // namespace stx2ast_pass
} // namespace exy

#endif // STX2AST_ISA_H_