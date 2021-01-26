//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef STX2AST_MK_H_
#define STX2AST_MK_H_

namespace exy {
namespace stx2ast_pass {
struct Make {
    using Node     = AstNode*;
    using Symbol   = AstSymbol*;
    using Constant = AstConstant*;
    using Type     = const AstType&;

    Typer &tp;

    Make(Typer *tp) : tp(*tp) {}

    Symbol importOf(Loc, Identifier name, Symbol);
    Symbol exportOf(Loc, Identifier name, Symbol);
    Symbol aliasOf(Loc, AstAliasKind decl, Identifier name, Symbol);

    Symbol global(Loc, Identifier name, Type type);

    Node name(Loc, AstSymbol*);
    Node tpname(Loc, Type);

    Node explicitCast(Loc, Node src, Type dst);
    Node implicitCast(Loc, Node src, Type dst);

    Node definition(Loc, Symbol symbol, Node rhs);

    Constant constant(Loc, Type, UINT64 = 0ui64);
    Constant void_(Loc);
    Constant null_(Loc);
    Constant char_(Loc, char);
    Constant bool_(Loc, bool);
    Constant wchar_(Loc, wchar_t);
    Constant utf8(Loc, BYTE u[4]);
    Constant int8(Loc,   INT8);
    Constant int16(Loc,  INT16);
    Constant int32(Loc,  INT32);
    Constant int64(Loc,  INT64);
    Constant uint8(Loc,  UINT8);
    Constant uint16(Loc, UINT16);
    Constant uint32(Loc, UINT32);
    Constant uint64(Loc, UINT64);
    Constant float32(Loc, float);
    Constant float64(Loc, double);
};
} // namespace stx2ast_pass
} // namespace exy

#endif // STX2AST_MK_H_