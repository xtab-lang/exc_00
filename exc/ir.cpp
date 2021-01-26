//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-23
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ir.h"

namespace exy {
//------------------------------------------------------------------------------------------------
IrType IrType::pointer() const {
    return { symbol, ptr + 1 };
}

IrType IrType::pointee() const {
    if (ptr) {
        return { symbol, ptr - 1 };
    }
    Unreachable();
}

IrTypeSymbol* IrType::isDirect() const {
    return ptr ? nullptr : symbol;
}
//------------------------------------------------------------------------------------------------
IrTree::IrTree() {
    comp.ir = this;
}
void IrTree::dispose() {
    ldispose(modules);
    mem.dispose();
}
//------------------------------------------------------------------------------------------------
void IrNode::dispose() {
    uses.dispose();
}

String IrNode::kindName() {
    return kindName(kind);
}

String IrNode::kindName(Kind k) {
    switch (k) {
    #define ZM(zName) case Kind::zName: return { S(#zName), 0u };
        DeclareIrNodes(ZM)
    #undef ZM
    }
    Unreachable();
}
//------------------------------------------------------------------------------------------------
void IrModule::dispose() {
    ldispose(symbols);
    entry = nullptr;
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
void IrFunction::dispose() {
    ldispose(body);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
void IrBlock::dispose() {
    ldispose(body);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
IrUse::IrUse(IrOperation *user, IrNode *value, IrUseKind useKind) 
    : user(user), value(value), useKind(useKind) {
    if (value) {
        value->uses.append(this);
    }
}
//------------------------------------------------------------------------------------------------
#define ZM(zName, zSize) IrType IrTree::ty##zName{};
DeclareBuiltinTypeKeywords(ZM)
#undef ZM
} // namespace exy