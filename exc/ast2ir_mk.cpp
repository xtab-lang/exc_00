//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-01-11
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ast2ir.h"

namespace exy {
namespace ast2ir_pass {
Make::Make(Scope *sc) : sc(*sc), mem(comp.ir->mem) {}

IrModule* Make::moduleOf(AstSymbol *ast) {
    auto scope = ast->ownScope ? ast->ownScope : ast->parentScope;
    while (scope) {
        if (scope->owner->kind == AstKind::Module) {
            return (IrModule*)symbolOf(scope->owner);
        }
        scope = scope->parent;
    }
    Unreachable();
}

IrSymbol* Make::symbolOf(AstSymbol *ast) {
    auto &symtab = sc.fn.parent.symtab;
    return symtab.get(ast);
}

IrType Make::typeOf(const AstType &type) {
    if (auto ptr = type.isaPointer()) {
        return typeOf(ptr->pointee).pointer();
    } if (auto ref = type.isaReference()) {
        return typeOf(ref->pointee).pointer();
    } if (auto ast = type.isDirect()) {
        return symbolOf(ast)->type;
    }
    Unreachable();
}

IrBlock* Make::block(Loc loc, Identifier name) {
    auto &freeBlocks = sc.fn.parent.freeBlocks;
    if (freeBlocks.length) {
        auto last = freeBlocks.pop();
        return new(last) IrBlock{ loc, name };
    }
    return mem.New<IrBlock>(loc, name);
}

IrConstant* Make::constant(Loc loc, Type type, UINT64 u64) {
    auto ir = mem.New<IrConstant>(loc, type, u64);
    sc.block->append(ir);
    return ir;
}

IrPath* Make::path(Loc loc, IrNode *base, IrNode *index) {
    auto ir = mem.New<IrPath>(loc, base, index);
    sc.block->append(ir);
    return ir;
}

IrNode* Make::assign(Loc loc, IrNode *lhs, IrNode *rhs) {
    Assert(lhs->type == rhs->type);
    auto ir = mem.New<IrAssign>(loc, lhs, rhs);
    sc.block->append(ir);
    return ir;
}

IrNode* Make::cast(Loc loc, Type dst, IrNode *src) {
    if (dst == src->type) {
        return src;
    }
    auto ir = mem.New<IrCast>(loc, dst, src);
    sc.block->append(ir);
    return ir;
}

} // namespace ast2ir_pass
} // namespace exy