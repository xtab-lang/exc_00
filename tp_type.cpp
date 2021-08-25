#include "pch.h"
#include "typer.h"

namespace exy {
TpType::TpType() : symbol(nullptr), kind(Kind::Unknown) {}

TpType::TpType(TpSymbol *symbol)
    : symbol(symbol), kind(Kind::Symbol) {

}

TpType::TpType(TpIndirectType *ptr, Kind kind) : ptr(ptr), kind(kind) {}

TpSymbol* TpType::isaBuiltin() const {
    if (kind == Kind::Symbol) {
        auto node = symbol->node;
        return node->kind == TpKind::Builtin ? symbol : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isNumeric() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return node->keyword >= Keyword::Char && node->keyword <= Keyword::Double ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isIntegral() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return node->keyword >= Keyword::Char && node->keyword <= Keyword::UInt64 ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isSigned() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return node->keyword >= Keyword::Char && node->keyword <= Keyword::Int64 ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isUnsigned() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return node->keyword >= Keyword::Bool && node->keyword <= Keyword::UInt64 ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaFloatingPoint() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return node->keyword == Keyword::Float || node->keyword == Keyword::Double ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isPacked() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return node->keyword >= Keyword::Floatx4 && node->keyword <= Keyword::UInt64x8 ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isPackedFloatingPoints() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return (node->keyword == Keyword::Floatx4 || node->keyword == Keyword::Doublex2) ||
            (node->keyword == Keyword::Floatx8 || node->keyword == Keyword::Doublex4) ||
            (node->keyword == Keyword::Floatx16 || node->keyword == Keyword::Doublex8)
            ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isPackedIntegrals() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return (node->keyword >= Keyword::Int8x16 && node->keyword <= Keyword::UInt64x2) ||
            (node->keyword >= Keyword::Int8x32 && node->keyword <= Keyword::UInt64x4) ||
            (node->keyword >= Keyword::Int8x64 && node->keyword <= Keyword::UInt64x8)
            ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isFloatingPointOrPackedFloatingPoints() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return (node->keyword == Keyword::Float || node->keyword == Keyword::Double) ||
            (node->keyword == Keyword::Floatx4 || node->keyword == Keyword::Doublex2) ||
            (node->keyword == Keyword::Floatx8 || node->keyword == Keyword::Doublex4) ||
            (node->keyword == Keyword::Floatx16 || node->keyword == Keyword::Doublex8)
            ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isIntegralOrPackedIntegrals() const {
    if (auto sym = isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return (node->keyword >= Keyword::Char && node->keyword <= Keyword::UInt64) ||
            (node->keyword >= Keyword::Int8x16 && node->keyword <= Keyword::UInt64x2) ||
            (node->keyword >= Keyword::Int8x32 && node->keyword <= Keyword::UInt64x4) ||
            (node->keyword >= Keyword::Int8x64 && node->keyword <= Keyword::UInt64x8)
            ? sym : nullptr;
    }
    return nullptr;
}

bool TpType::isNumericOrIndirect() const {
    return isIndirect() || isaBuiltin();
}

TpSymbol* TpType::isaModule() const {
    if (auto  sym = isDirect()) {
        auto node = sym->node;
        return node->kind == TpKind::Module ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isOverloadSet() const {
    if (auto  sym = isDirect()) {
        auto node = sym->node;
        return node->kind == TpKind::OverloadSet ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaTemplate() const {
    if (auto  sym = isDirect()) {
        auto node = sym->node;
        return node->kind == TpKind::Template ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaStruct() const {
    if (auto  sym = isDirect()) {
        auto node = sym->node;
        return node->kind == TpKind::Struct ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaStructTemplate() const {
    if (auto  sym = isaTemplate()) {
        auto node = (TpTemplate*)sym->node;
        return node->syntax->pos.keyword == Keyword::Struct ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaUnion() const {
    if (auto  sym = isDirect()) {
        auto node = sym->node;
        return node->kind == TpKind::Union ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaUnionTemplate() const {
    if (auto  sym = isaTemplate()) {
        auto node = (TpTemplate*)sym->node;
        return node->syntax->pos.keyword == Keyword::Union ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isanEnum() const {
    if (auto  sym = isDirect()) {
        auto node = sym->node;
        return node->kind == TpKind::Enum ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isanEnumTemplate() const {
    if (auto  sym = isaTemplate()) {
        auto node = (TpTemplate*)sym->node;
        return node->syntax->pos.keyword == Keyword::Enum ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaStructOrUnionOrEnum() const {
    if (auto  sym = isDirect()) {
        auto node = sym->node;
        return node->kind == TpKind::Struct || node->kind == TpKind::Union || node->kind == TpKind::Enum ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaFunction() const {
    if (auto  sym = isDirect()) {
        auto node = sym->node;
        return node->kind == TpKind::Function ? sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaFunctionTemplate() const {
    if (auto  sym = isaTemplate()) {
        auto node = (TpTemplate*)sym->node;
        return node->syntax->pos.keyword >= Keyword::Fn && node->syntax->pos.keyword <= Keyword::Blob ?
            sym : nullptr;
    }
    return nullptr;
}

TpSymbol* TpType::isaFunctionOrFunctionTemplate() const {
    if (auto  sym = isDirect()) {
        auto node = sym->node;
        if (node->kind == TpKind::Function) {
            return sym;
        }
        if (node->kind == TpKind::Template) {
            auto t = (TpTemplate*)node;
            return t->syntax->pos.keyword >= Keyword::Fn && t->syntax->pos.keyword <= Keyword::Blob ?
                sym : nullptr;
        }
    }
    return nullptr;
}

TpType TpType::mkPointer() const {
    return typer->_types.pointerOf(*this);
}

TpType TpType::mkReference() const {
    return typer->_types.referenceOf(*this);
}

TpType TpType::pointee() const {
    Assert(isIndirect());
    return ptr->pointee;
}

TpType TpType::dereference() const {
    if (isIndirect()) {
        return ptr->pointee;
    }
    return *this;
}

TpType TpType::dereferenceIfReference() const {
    if (isaReference()) {
        return ptr->pointee;
    }
    return *this;
}

TpIndirectType* TpType::isVoidPointer() const {
    if (auto p = isIndirect()) {
        return p->pointee.isVoid() ? p : nullptr;
    }
    return nullptr;
}

#define ZM(zName, zSize) TpSymbol* TpType::is##zName() const { if (auto s = isaBuiltin()) { auto b = (TpBuiltin*)s->node; return b->keyword == Keyword::zName ? s : nullptr; } return nullptr; }
DeclareBuiltinTypeKeywords(ZM)
#undef ZM

//----------------------------------------------------------
void TpIndirectTypes::initialize() {
    auto &tree = *compiler.tpTree;
    auto  &mem = tree.mem;
    const auto length = INT(Keyword::UInt64x8) - INT(Keyword::Void);
    builtins.reserve(length);
#define ZM(zName, zSize) builtins.append(mem.New<TpIndirectType>(tree.ty##zName));
    DeclareBuiltinTypeKeywords(ZM)
    #undef ZM
        builtins.length = length;
    builtins.compact();
    tree.tyVoidPointer = tree.tyVoid.mkPointer();
}

void TpIndirectTypes::dispose() {
    builtins.dispose();
    ptrs.dispose();
}

TpType TpIndirectTypes::pointerOf(const TpType &type) {
    auto &tree = *compiler.tpTree;
    auto  &mem = tree.mem;
    if (auto  symbol = type.isaBuiltin()) {
        auto builtin = (TpBuiltin*)symbol->node;
        auto   index = INT(builtin->keyword) - INT(Keyword::Void);
        return { builtins.items[index], TpType::Kind::Pointer };
    }
    if (auto symbol = type.isDirect()) {
        auto   hash = UINT64(symbol);
        auto    idx = ptrs.indexOf(hash);
        if (idx >= 0) {
            return { ptrs.items[idx].value, TpType::Kind::Pointer };
        }
        auto ptr = mem.New<TpIndirectType>(symbol->node->type);
        ptrs.append(hash, ptr);
        return { ptr, TpType::Kind::Pointer };
    }
    if (auto  ptr = type.isIndirect()) {
        auto hash = UINT64(ptr);
        auto  idx = ptrs.indexOf(hash);
        if (idx >= 0) {
            return { ptrs.items[idx].value, TpType::Kind::Pointer };
        }
        ptr = mem.New<TpIndirectType>(TpType(ptr, TpType::Kind::Pointer));
        ptrs.append(hash, ptr);
        return { ptr, TpType::Kind::Pointer };
    }
    UNREACHABLE();
}

TpType TpIndirectTypes::referenceOf(const TpType &type) {
    auto &tree = *compiler.tpTree;
    auto  &mem = tree.mem;
    if (auto symbol = type.isaBuiltin()) {
        auto builtin = (TpBuiltin*)symbol->node;
        auto   index = INT(builtin->keyword) - INT(Keyword::Void);
        return { builtins.items[index], TpType::Kind::Reference };
    }
    if (auto symbol = type.isDirect()) {
        auto   hash = UINT64(symbol);
        auto    idx = ptrs.indexOf(hash);
        if (idx >= 0) {
            return { ptrs.items[idx].value, TpType::Kind::Reference };
        }
        auto ptr = mem.New<TpIndirectType>(symbol->node->type);
        ptrs.append(hash, ptr);
        return { ptr, TpType::Kind::Reference };
    }
    if (auto  ptr = type.isIndirect()) {
        auto hash = UINT64(ptr);
        auto  idx = ptrs.indexOf(hash);
        if (idx >= 0) {
            return { ptrs.items[idx].value, TpType::Kind::Reference };
        }
        ptr = mem.New<TpIndirectType>(TpType(ptr, TpType::Kind::Reference));
        ptrs.append(hash, ptr);
        return { ptr, TpType::Kind::Reference };
    }
    UNREACHABLE();
}
} // namespace exy