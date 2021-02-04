//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-12
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ast.h"
#include "syntax.h"

#define err(token, msg, ...) print_error("AST", token, msg, __VA_ARGS__)

namespace exy {
//------------------------------------------------------------------------------------------------
#define ZM(zName, zSize) AstType AstTree::ty##zName{};
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
AstType AstTree::tyNull{};
//------------------------------------------------------------------------------------------------
void AstTree::initialize(Loc loc) {
    List<SyntaxFile*> noFiles{};
    global = mem.New<AstModule>(loc, nullptr, ids.main, ids.main, noFiles, nullptr, BinaryKind::Console);
    auto scope = global->ownScope;
#define ZM(zName, zSize) ty##zName = mem.New<AstBuiltin>(loc, scope, ids.get(S(#zName)), Keyword::zName );
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
    tyNull = tyVoid.pointer();

    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, scope, ids.get(S("SByte")), tyInt8);
    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, scope, ids.get(S("Byte")), tyUInt8);

    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, scope, ids.get(S("Short")), tyInt16);
    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, scope, ids.get(S("UShort")), tyUInt16);

    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, scope, ids.get(S("Int")), tyInt32);
    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, scope, ids.get(S("UInt")), tyUInt32);

    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, scope, ids.get(S("Long")), tyInt64);
    mem.New<AstTypeAlias>(loc, AstAliasKind::Let, scope, ids.get(S("ULong")), tyUInt64);
}

void AstTree::dispose() {
    global = ndispose(global);
    ptrs.dispose();
    refs.dispose();
    mem.dispose();
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
AstScope::AstScope(Loc open, Loc close, ScopeOwner owner, ParentScope parent) 
    : AstNode(owner->loc, Kind::Scope, owner->type), open(open), close(close), owner(owner), 
    parent(parent) {}
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

void AstScope::append(Node node) {
    if (node) {
        nodes.append(node);
    }
}
//------------------------------------------------------------------------------------------------
AstSymbol::AstSymbol(Loc loc, Kind kind, Type type, ParentScope parentScope, Identifier name)
    : AstNode(loc, kind, type), parentScope(parentScope), name(name) {
    if (parentScope) {
        parentScope->symbols.append(name, this);
    }
}
void AstSymbol::dispose() {
    ownScope = ndispose(ownScope);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
AstModule::AstModule(Loc loc, ParentScope parent, Identifier name, Identifier dotName,
                     List<SyntaxFile*> &files, SyntaxFile *main, BinaryKind binaryKind) 
    : AstSymbol(loc, Kind::Module, AstType(this), parent, name), dotName(dotName), binaryKind(binaryKind) {
    auto &mem = comp.ast->mem;
    if (files.isNotEmpty()) {
        if (!main) {
            main = files.first();
        }
        ownScope = mem.New<AstScope>(main->tokens().first().loc, main->tokens().last().loc,
                                  this, parent);
    } else {
        ownScope = mem.New<AstScope>(/* open = */ loc, /* close = */ loc, this, parent);
    }
    for (auto i = 0; i < files.length; ++i) {
        syntax.append(files.items[i]);
    }
}

void AstModule::dispose() {
    syntax.dispose();
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
AstDefinition::AstDefinition(Loc loc, AstName *lhs, Node rhs) 
    : AstNode(loc, Kind::Definition, lhs->type), lhs(lhs), rhs(rhs) {
    Assert(lhs->type == rhs->type);
}

void AstDefinition::dispose() {
    lhs = ndispose(lhs);
    rhs = ndispose(rhs);
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