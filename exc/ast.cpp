//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-12
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ast.h"

#define err(token, msg, ...) print_error("AST", token, msg, __VA_ARGS__)

namespace exy {
//------------------------------------------------------------------------------------------------
#define ZM(zName, zSize) AstType AstTree::ty##zName{};
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
AstType AstTree::tyNull{};
//------------------------------------------------------------------------------------------------
void AstTree::initialize(Loc loc) {
#define ZM(zName, zSize) ty##zName = mem.New<AstBuiltin>(loc, ids.get(S(#zName)), Keyword::zName );
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
    tyNull = tyVoid.pointer();

    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, nullptr, ids.get(S("SByte")), tyInt8);
    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, nullptr, ids.get(S("Byte")), tyUInt8);

    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, nullptr, ids.get(S("Short")), tyInt16);
    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, nullptr, ids.get(S("UShort")), tyUInt16);

    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, nullptr, ids.get(S("Int")), tyInt32);
    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, nullptr, ids.get(S("UInt")), tyUInt32);

    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, nullptr, ids.get(S("Long")), tyInt64);
    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, nullptr, ids.get(S("ULong")), tyUInt64);
}
void AstTree::dispose() {
    ldispose(symbols);
    ptrs.dispose();
    refs.dispose();
    mem.dispose();
}

AstSymbol* AstTree::find(Identifier name) {
    auto idx = symbols.indexOf(name);
    if (idx >= 0) {
        return symbols.items[idx].value;
    }
    return nullptr;
}
//------------------------------------------------------------------------------------------------
String AstNode::kindName() const {
    return kindName(kind);
}

String AstNode::kindName(Kind k) {
    switch (k) {
    #define ZM(zName) case AstKind::zName: return { S(#zName), 0u };
        DeclareAstNodes(ZM)
    #undef ZM
    }
    Unreachable();
}
//------------------------------------------------------------------------------------------------
AstScope::AstScope(AstModule *owner)
    : AstNode(owner->loc, Kind::Scope, AstType()), owner(owner) {}
AstScope::AstScope(ScopeOwner owner, ParentScope parent) 
    : AstNode(owner->loc, Kind::Scope, comp.ast->tyNull), owner(owner), parent(parent) {}
void AstScope::dispose() {
    ldispose(others);
    ldispose(symbols);
    ldispose(nodes);
    __super::dispose();
}
AstSymbol* AstScope::find(Identifier name) {
    auto idx = symbols.indexOf(name);
    if (idx >= 0) {
        return symbols.items[idx].value;
    }
    return nullptr;
}
AstSymbol* AstScope::findThroughDot(Identifier name) {
    if (auto found = find(name)) {
        switch (found->kind) {
            case Kind::TypeAlias:
            case Kind::ValueAlias:
            case Kind::ConstAlias: {
                auto alias = (AstAlias*)found;
                if (alias->decl == AstAliasKind::Import) {
                    return nullptr;
                }
                return alias; // Export | TyParam | Let
            }
            default:
                return found;
        }

    }
    return nullptr;
}

Identifier AstScope::name() {
    return owner ? owner->name : comp.str.block;
}

void AstScope::append(Node node) {
    if (node) {
        nodes.append(node);
    }
}
//------------------------------------------------------------------------------------------------
AstSymbol::AstSymbol(Loc loc, Kind kind, Type type, ParentScope parent, Identifier name)
    : AstNode(loc, kind, type), parent(parent), name(name) {
    if (parent) {
        parent->symbols.append(name, this);
    } else {
        comp.ast->symbols.append(name, this);
    }
}
void AstSymbol::dispose() {
    scope = ndispose(scope);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
AstBuiltin::AstBuiltin(Loc loc, Identifier name, Keyword keyword) 
    : AstSymbol(loc, Kind::Builtin, AstType(this), nullptr, name), keyword(keyword) {
    status.value = Status::Done;
}
//------------------------------------------------------------------------------------------------
AstModule::AstModule(Loc loc, Identifier name, Identifier dotName,
                     List<SyntaxFile*> &files) 
    : AstSymbol(loc, Kind::Module, AstType(this), nullptr, name), dotName(dotName) {
    auto &mem = comp.ast->mem;
    scope = mem.New<AstScope>(this, parent);
    for (auto i = 0; i < files.length; ++i) {
        syntax.append(files.items[i]);
    }
}
AstModule::AstModule(Loc loc, ParentScope parent, Identifier name, Identifier dotName,
                     List<SyntaxFile*> &files) 
    : AstSymbol(loc, Kind::Module, AstType(this), parent, name), dotName(dotName) {
    auto &mem = comp.ast->mem;
    scope = mem.New<AstScope>(this, parent);
    for (auto i = 0; i < files.length; ++i) {
        syntax.append(files.items[i]);
    }
}
void AstModule::dispose() {
    syntax.dispose();
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
AstConstAlias::AstConstAlias(Loc loc, AstAliasKind decl, ParentScope parent, Identifier name, AstConstant *value) 
    : AstAlias(loc, Kind::ConstAlias, decl, type, parent, name), value(value) {}
//------------------------------------------------------------------------------------------------
void AstGlobal::dispose() {
    value = ndispose(value);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
void AstBinary::dispose() {
    lhs = ndispose(lhs);
    rhs = ndispose(rhs);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
void AstCast::dispose() {
    value = ndispose(value);
    __super::dispose();
}
} // namespace exy