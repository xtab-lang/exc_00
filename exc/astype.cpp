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

bool AstType::operator==(const AstType &other) const {
    if (kind == other.kind) {
        if (symbol == other.symbol) {
            return true;
        }
    }
    return false;
}

bool AstType::operator!=(const AstType &other) const {
    return !this->operator==(other);
}

AstSymbol* AstType::isaSymbol() const {
    return isDirect();
}

AstSymbol* AstType::isDirect() const {
    return kind == Kind::Direct ? symbol : nullptr;;
}

bool AstType::isIndirect() const {
    return kind != Kind::Direct;
}

AstPointerType* AstType::isaPointer() const {
    return kind == Kind::Pointer ? ptr : nullptr;
}

AstReferenceType* AstType::isaReference() const {
    return kind == Kind::Reference ? ref : nullptr;
}
} // namespace exy