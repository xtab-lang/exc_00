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
    ZM(Module)                      \
    /* Aliases */                   \
    ZM(TypeAlias)

#define DeclareAstValueSymbols(ZM)  \
    /* Aliases */                   \
    ZM(ValueAlias)                  \
    ZM(ConstAlias)                  \
    /* Variables */                 \
    ZM(Global)                      \
    ZM(Local)                       \
    ZM(FnParam)                     \
    ZM(Field)

#define DeclareAstSymbols(ZM)   \
    DeclareAstTypeSymbols(ZM)   \
    DeclareAstValueSymbols(ZM)

#define DeclareAstNames(ZM)     \
    ZM(Name)                    \
    ZM(TypeName)
struct AstName;

#define DeclareAstNodes(ZM)     \
    ZM(Scope)                   \
    ZM(Binary)                  \
    ZM(Cast)                    \
    ZM(Constant)                \
    /* Symbols */               \
    DeclareAstSymbols(ZM)       \
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
    Dict<AstSymbol*>      symbols;
    Dict<AstType, UINT64> ptrs;
    Dict<AstType, UINT64> refs;

#define ZM(zName, zSize) static AstType ty##zName;
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
    static AstType tyNull;

    void initialize(Loc);
    void dispose();
    AstSymbol* find(Identifier);
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

    Dict<AstSymbol*> symbols;
    List<AstSymbol*> others;
    Nodes            nodes;
    ScopeOwner       owner;
    ParentScope      parent;

    AstScope(AstModule *owner);
    AstScope(ScopeOwner owner, ParentScope parent);
    void dispose() override;
    AstSymbol* find(Identifier);
    AstSymbol* findThroughDot(Identifier);
    Identifier name();
    void append(Node);
};
//------------------------------------------------------------------------------------------------
// Symbols
struct AstSymbol : AstNode {
    struct Status {
        enum Value { None, Busy, Done };
        Value value;

        bool isIdle() const { return value == None; }
        bool isBusy() const { return value == Busy; }
        bool isDone() const { return value == Done; }

        void begin() { Assert(isIdle()); value = Busy; }
        void end()   { Assert(isBusy()); value = Done; }
    };
    using ParentScope = AstScope*;
    using OwnScope    = AstScope*;

    ParentScope  parent; // The scope containing this symbol.
    OwnScope     scope;  // The scope created by this symbol.
    Identifier   name;
    Status       status;
    int          padding; // Padding after symbol in scope.

    AstSymbol(Loc loc, Kind kind, Type type, ParentScope parent, Identifier name);
    void dispose() override;
};

struct AstBuiltin : AstSymbol {
    Keyword keyword;

    AstBuiltin(Loc loc, Identifier name, Keyword keyword);
};

struct AstModule : AstSymbol {
    Identifier        dotName;
    List<SyntaxFile*> syntax;

    AstModule(Loc loc, Identifier name, Identifier dotName, List<SyntaxFile*>&);
    AstModule(Loc loc, ParentScope parent, Identifier name, Identifier dotName, List<SyntaxFile*>&);
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

    AstConstAlias(Loc loc, AstAliasKind decl, ParentScope parent, Identifier name, AstConstant *value);
};
//------------------------------------------------------------------------------------------------
// Variables
struct AstVariable : AstSymbol {
    AstVariable(Loc loc, Kind kind, Type type, ParentScope parent, Identifier name) 
        : AstSymbol(loc, kind, type, parent, name) {}
};

struct AstGlobal : AstVariable {
    Node value;
    AstGlobal(Loc loc, Type type, ParentScope parent, Identifier name) 
        : AstVariable(loc, Kind::Global, type, parent, name) {}
    AstGlobal(Loc loc, ParentScope parent, Identifier name, Node value) 
        : AstVariable(loc, Kind::Global, value->type, parent, name), value(value) {}
    void dispose() override;
};

struct AstLocal : AstVariable {
    AstLocal(Loc loc, Type type, ParentScope parent, Identifier name) 
        : AstVariable(loc, Kind::Local, type, parent, name) {}
};

struct AstFnParam : AstVariable {
    AstFnParam(Loc loc, Type type, ParentScope parent, Identifier name) 
        : AstVariable(loc, Kind::FnParam, type, parent, name) {}
};

struct AstField : AstVariable {
    AstField(Loc loc, Type type, ParentScope parent, Identifier name) 
        : AstVariable(loc, Kind::Field, type, parent, name) {}
};
//------------------------------------------------------------------------------------------------
// Binary operations.
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

struct AstVoid : AstConstant {
    AstVoid(Loc loc) 
        : AstConstant(loc, AstType(comp.ast->tyVoid), 0ui64) {}
};

struct AstNull : AstConstant {
    AstNull(Loc loc) 
        : AstConstant(loc, comp.ast->tyNull, 0ui64) {}
};

struct AstChar : AstConstant {
    AstChar(Loc loc, char ch) 
        : AstConstant(loc, comp.ast->tyChar, UINT64(UINT8(ch))) {}
};

struct AstBool : AstConstant {
    AstBool(Loc loc, bool b) 
        : AstConstant(loc, comp.ast->tyBool, UINT64(UINT8(b))) {}
};

struct AstWChar : AstConstant {
    AstWChar(Loc loc, wchar_t ch) 
        : AstConstant(loc, comp.ast->tyWChar, UINT64(UINT16(ch))) {}
};

struct AstUtf8 : AstConstant {
    AstUtf8(Loc loc, UINT32 u32) 
        : AstConstant(loc, comp.ast->tyUtf8, UINT64(u32)) {}
};

struct AstSignedInt : AstConstant {
    AstSignedInt(Loc loc, Type type, INT64 i64) 
        : AstConstant(loc, type, UINT64(i64)) {}
};

struct AstUnsignedInt : AstConstant {
    AstUnsignedInt(Loc loc, Type type, UINT64 u64) 
        : AstConstant(loc, type, u64) {}
};

struct AstFloat : AstConstant {
    AstFloat(Loc loc, float f32) 
        : AstConstant(loc, comp.ast->tyFloat, UINT64(meta::reinterpret<UINT32>(f32))) {}
};

struct AstDouble : AstConstant {
    AstDouble(Loc loc, double f64) 
        : AstConstant(loc, comp.ast->tyDouble, meta::reinterpret<UINT64>(f64)) {}
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