//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ast.h"

namespace exy {
AstType AstType::pointer() const {
    Assert(isKnown());
    auto &tree = *comp.ast;
    auto  hash = UINT64(symbol);
    auto   idx = tree.ptrs.indexOf(hash);
    if (idx >= 0) {
        return tree.ptrs.items[idx].value;
    }
    AstType type{ tree.mem.New<AstPointerType>(*this) };
    tree.ptrs.append(hash, type);
    return type;
}

AstType AstType::reference() const {
    Assert(isKnown());
    auto &tree = *comp.ast;
    auto  hash = UINT64(symbol);
    auto   idx = tree.refs.indexOf(hash);
    if (idx >= 0) {
        return tree.refs.items[idx].value;
    }
    AstType type{ tree.mem.New<AstReferenceType>(*this) };
    tree.refs.append(hash, type);
    return type;
}
} // namespace exy