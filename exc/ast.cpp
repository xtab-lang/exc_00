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
    switch (kind) {
    #define ZM(zName, zText) case AstKind::zName: return { S(#zName), 0u };
        DeclareAstNodes(ZM)
    #undef ZM
    }
    Unreachable();
}

String AstNode::kindValue() const {
    switch (kind) {
    #define ZM(zName, zText) case AstKind::zName: return { S(zText), 0u };
        DeclareAstNodes(ZM)
    #undef ZM
    }
    Unreachable();
}

AstSymbol* AstNode::isaSymbol() {
    switch (kind) {
    #define ZM(zName, zText) case AstKind::zName: return (AstSymbol*)this;
        DeclareAstSymbols(ZM)
    #undef ZM
    }
    return nullptr;
}

AstName* AstNode::isaName() {
    switch (kind) {
    #define ZM(zName, zText) case AstKind::zName: return (AstName*)this;
        DeclareAstNames(ZM)
    #undef ZM
    }
    return nullptr;
}
//------------------------------------------------------------------------------------------------
AstScope::AstScope(AstModule *owner)
    : AstNode(owner->loc, Kind::Scope, AstType()), owner(owner) {}
AstScope::AstScope(ScopeOwner owner, ParentScope parent) 
    : AstNode(owner->loc, Kind::Scope, AstType()), owner(owner), parent(parent) {}
void AstScope::dispose() {
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
        if (found->kind != AstKind::Import) {
            return found;
        }
    }
    return nullptr;
}

Identifier AstScope::name() {
    return owner ? owner->name : comp.str.block;
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
} // namespace exy