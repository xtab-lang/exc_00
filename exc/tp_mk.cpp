//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(token, msg, ...) print_error("Make", token, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
AstAlias* Make::importOf(Loc loc, Identifier name, AstSymbol *symbol) {
    return alias(loc, AstAliasKind::Import, name, symbol);
}

AstAlias* Make::exportOf(Loc loc, Identifier name, AstSymbol *symbol) {
    return alias(loc, AstAliasKind::Export, name, symbol);
}

AstAlias* Make::alias(Loc loc, AstAliasKind decl, Identifier name, AstSymbol *symbol) {
    auto scope = tp.currentScope();
    switch (symbol->kind) {
        case AstKind::Builtin:
        case AstKind::Module:
        case AstKind::TypeAlias:
            return tp.mem.New<AstTypeAlias>(loc, decl, scope, name, symbol->type);

        case AstKind::ValueAlias:
            return alias(loc, decl, name, ((AstValueAlias*)symbol)->value);

        case AstKind::ConstAlias:
            return tp.mem.New<AstConstAlias>(loc, decl, scope, name, ((AstConstAlias*)symbol)->value);

        case AstKind::Global:
        case AstKind::Local:
        case AstKind::FnParam:
            return tp.mem.New<AstValueAlias>(loc, decl, scope, name, symbol);
    } 

    err(loc, "illegal alias of %ast symbol", symbol->kind);

    return nullptr;
}

AstGlobal* Make::global(Loc loc, Identifier name, const AstType &type) {
    auto scope = tp.currentScope();
    if (auto found = scope->find(name)) {
        err(loc, "%s#<red> already defined in the current scope", name);
        return nullptr;
    }
    return tp.mem.New<AstGlobal>(loc, type, scope, name);
}

AstGlobal* Make::global(Loc loc, Identifier name, AstNode* rhs) {
    auto scope = tp.currentScope();
    if (auto found = scope->find(name)) {
        err(loc, "%s#<red> already defined in the current scope", name);
        return (AstGlobal*)tp.throwAway(rhs);
    }
    return tp.mem.New<AstGlobal>(loc, scope, name, rhs);
}

AstNode* Make::name(Loc loc, AstSymbol *symbol) {
    switch (symbol->kind) {
        case AstKind::Builtin:
        case AstKind::Module:
        case AstKind::TypeAlias:
            return tpname(loc, symbol->type);

        case AstKind::ValueAlias: {
            symbol = ((AstValueAlias*)symbol)->value;
        } break;

        case AstKind::ConstAlias:
        case AstKind::Global:
        case AstKind::Local:
        case AstKind::FnParam:
            break; // OK. Will make name instead of tpname.

        default: {
            err(loc, "cannot make name of %ast symbol", symbol->kind);
            symbol = nullptr;
        } break;
    } 
    
    if (symbol) {
        if (tp._names.length) {
            auto last = tp._names.pop();
            return new(last) AstName{ loc, symbol };
        }
        return tp.mem.New<AstName>(loc, symbol);
    }
    
    return nullptr;
}

AstTypeName* Make::tpname(Loc loc, const AstType &type) {
    if (tp._tpnames.length) {
        auto last = tp._tpnames.pop();
        return new(last) AstTypeName{ loc, type };
    }
    return tp.mem.New<AstTypeName>(loc, type);
}

AstNode* Make::explicitCast(Loc loc, AstNode *src, const AstType &dst) {
    return Cast(tp).explicitCast(loc, src, dst);
}

AstNode* Make::implicitCast(Loc loc, AstNode *src, const AstType &dst) {
    return Cast(tp).implicitCast(loc, src, dst);
}
} // namespace typ_pass
} // namespace exy