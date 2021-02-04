//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-01-11
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef AST2IR_MK_H
#define AST2IR_MK_H

namespace exy {
namespace ast2ir_pass {
struct Scope;

struct Make {
    using Type = const IrType&;
    Scope &sc;
    Mem   &mem;

    Make(Scope *sc);

    IrModule* moduleOf(AstSymbol*);
    IrSymbol* symbolOf(AstSymbol*);
    IrType typeOf(const AstType&);
    IrBlock* block(Loc, Identifier = ids.block);

    IrNode* exit(Loc);
    IrNode* constant(Loc, Type, UINT64 u64);
    IrNode* path(Loc, IrNode *base, IrNode *index);
    IrNode* assign(Loc, IrNode *lhs, IrNode *rhs);
    IrNode* cast(Loc, Type dst, IrNode *src);
};
} // namespace ast2ir_pass
} // namespace exy

#endif // AST2IR_MK_H