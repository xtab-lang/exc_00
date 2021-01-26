//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-12
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef AST_H
#define AST_H

#include "astype.h"

namespace exy {
//--Begin forward declarations
struct SyntaxFile;

struct AstNode;

#define DeclareAstTypeSymbols(ZM)   \
    ZM(Builtin)                     \
    ZM(Module)

#define DeclareAstAliasSymbols(ZM)  \
    ZM(TypeAlias)                   \
    ZM(ValueAlias)                  \
    ZM(ConstAlias)
struct AstAlias;

#define DeclareAstValueSymbols(ZM)  \
    ZM(Global)

#define DeclareAstSymbols(ZM)   \
    DeclareAstTypeSymbols(ZM)   \
    DeclareAstAliasSymbols(ZM)  \
    DeclareAstValueSymbols(ZM)

#define DeclareAstNames(ZM)     \
    ZM(Name)                    \
    ZM(TypeName)

#define DeclareAstNodes(ZM)     \
    /* Symbols */               \
    DeclareAstSymbols(ZM)       \
    /* Scope */                 \
    ZM(Scope)                   \
    /* Operations */            \
    ZM(Definition)              \
    ZM(Binary)                  \
    ZM(Cast)                    \
    /* Constants */             \
    ZM(Constant)                \
    /* Names */                 \
    DeclareAstNames(ZM)

enum class AstKind {
#define ZM(zName) zName,
    DeclareAstNodes(ZM)
#undef ZM
};

#define ZM(zName) struct Ast##zName;
    DeclareAstNodes(ZM)
#undef ZM
//----End forward declarations

//------------------------------------------------------------------------------------------------
struct AstTree {
    Mem                   mem;
    AstModule            *global;
    Dict<AstType, UINT64> ptrs;
    Dict<AstType, UINT64> refs;

#define ZM(zName, zSize) static AstType ty##zName;
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
    static AstType tyNull;

    void initialize(Loc);
    void dispose();
};

struct AstNode {
    using   Kind = AstKind;
    using   Node = AstNode*;
    using  Nodes = List<Node>;
    using   Type = const AstType&;
    using Symbol = AstSymbol*;

    SourceLocation  loc;
    Kind            kind;
    AstType         type;

    AstNode(Loc loc, Kind kind, Type type) : loc(loc), kind(kind), type(type) { Assert(type.isKnown());  }
    virtual void dispose() {}
    String kindName() const;
    static String kindName(Kind);
};

struct AstScope : AstNode {
    using ParentScope = AstScope*;
    using ScopeOwner  = Symbol;

    SourceLocation   open;
    SourceLocation   close;
    Dict<AstSymbol*> symbols;
    List<AstSymbol*> others;
    Nodes            nodes;
    ScopeOwner       owner;
    ParentScope      parent;
    Status           status;

    AstScope(Loc open, Loc close, ScopeOwner owner, ParentScope parent);
    void dispose() override;
    AstSymbol* find(Identifier);
    AstSymbol* findThroughDot(Identifier);
    void append(Node);
    bool isEmpty() { return nodes.isEmpty(); }
    bool isNotEmpty() { return nodes.isNotEmpty(); }
};
//------------------------------------------------------------------------------------------------
// Symbols
struct AstSymbol : AstNode {
    using ParentScope = AstScope*;
    using OwnScope    = AstScope*;

    ParentScope  parentScope; // The scope containing this symbol.
    OwnScope     ownScope;    // The scope created by this symbol.
    Identifier   name;

    AstSymbol(Loc loc, Kind kind, Type type, ParentScope parentScope, Identifier name);
    void dispose() override;
};

struct AstBuiltin : AstSymbol {
    Keyword keyword;

    AstBuiltin(Loc loc, ParentScope parent, Identifier name, Keyword keyword)
        : AstSymbol(loc, Kind::Builtin, AstType(this), parent, name), keyword(keyword) {}
};

struct AstModule : AstSymbol {
    Identifier        dotName;
    List<SyntaxFile*> syntax;

    AstModule(Loc loc, ParentScope parent, Identifier name, Identifier dotName, 
              List<SyntaxFile*>&, SyntaxFile *main);
    void dispose() override;
};
//------------------------------------------------------------------------------------------------
// Aliases
enum class AstAliasKind {
    Import, Export, TyParam, Let
};

struct AstAlias : AstSymbol {
    AstAliasKind decl;

    AstAlias(Loc loc, Kind kind, AstAliasKind decl, Type type, ParentScope parent, Identifier name) 
        : AstSymbol(loc, kind, type, parent, name), decl(decl) {}
};

struct AstTypeAlias : AstAlias {
    AstTypeAlias(Loc loc, AstAliasKind decl, ParentScope parent, Identifier name, Type type)
        : AstAlias(loc, Kind::TypeAlias, decl, type, parent, name) {}
};

struct AstValueAlias : AstAlias {
    Symbol value;

    AstValueAlias(Loc loc, AstAliasKind decl, ParentScope parent, Identifier name, Symbol value)
        : AstAlias(loc, Kind::ValueAlias, decl, value->type, parent, name), value(value) {}
};

struct AstConstAlias : AstAlias {
    AstConstant *value;

    AstConstAlias(Loc loc, AstAliasKind decl, ParentScope parent, Identifier name, AstConstant *value) 
        : AstAlias(loc, Kind::ConstAlias, decl, type, parent, name), value(value) {}
};
//------------------------------------------------------------------------------------------------
// Variables
struct AstVariable : AstSymbol {
    AstVariable(Loc loc, Kind kind, Type type, ParentScope parent, Identifier name) 
        : AstSymbol(loc, kind, type, parent, name) {}
};

struct AstGlobal : AstVariable {
    AstGlobal(Loc loc, Type type, ParentScope parent, Identifier name) 
        : AstVariable(loc, Kind::Global, type, parent, name) {}
};
//------------------------------------------------------------------------------------------------
// Definition.
struct AstDefinition : AstNode {
    AstName *lhs;
    Node     rhs;

    AstDefinition(Loc loc, AstName *lhs, Node rhs);
    void dispose() override;
};
//------------------------------------------------------------------------------------------------
// Non-definition binary operations.
struct AstBinary : AstNode {
    Node lhs;
    Node rhs;
    Tok  op;

    AstBinary(Loc loc, Type type, Node lhs, Tok op, Node rhs) 
        : AstNode(loc, Kind::Binary, type), lhs(lhs), op(op), rhs(rhs) {}
    void dispose() override;
};

struct AstAssign : AstBinary {
    AstAssign(Loc loc, Node lhs, Node rhs) 
        : AstBinary(loc, lhs->type, lhs, Tok::Assign, rhs) {
        Assert(lhs->type == rhs->type);
    }
};
//------------------------------------------------------------------------------------------------
// Cast operations.
struct AstCast : AstNode {
    Node value;

    AstCast(Loc loc, Node value, Type type) 
        : AstNode(loc, Kind::Cast, type), value(value) {}
    void dispose() override;
};
//------------------------------------------------------------------------------------------------
// Constants.
struct AstConstant : AstNode {
    union {
        char   ch;
        bool   b;
        INT8   i8;
        UINT8  u8;
        INT32  i16;
        UINT16 u16;
        INT32  i32;
        UINT32 u32;
        INT64  i64;
        UINT64 u64;
        BYTE   utf8[4];
        float  f32;
        double f64;
    }; 
    AstConstant(Loc loc, Type type, UINT64 u64) 
        : AstNode(loc, Kind::Constant, type), u64(u64) {}
};
//------------------------------------------------------------------------------------------------
// Names
struct AstName : AstNode {
    AstSymbol *symbol;

    AstName(Loc loc, AstSymbol *symbol) 
        : AstNode(loc, Kind::Name, symbol->type), symbol(symbol) {}
};

struct AstTypeName : AstNode {
    AstTypeName(Loc loc, Type type) 
        : AstNode(loc, Kind::TypeName, type) {}
};
} // namespace exy

#endif // AST_H