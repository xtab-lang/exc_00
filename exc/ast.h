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

#define DeclareAstSymbols(ZM)   \
    ZM(Builtin, "builtin")      \
    ZM(Module,  "module")       \
    /* Aliases */               \
    ZM(Import,  "import")       \
    ZM(Export,  "export")       \
    ZM(TyParam, "typaram")      \
    /* Variables */             \
    ZM(Global,  "global")       \
    ZM(Local,   "local")        \
    ZM(FnParam, "fnparam")      \
    ZM(Field,   "field")
struct AstSymbol;

#define DeclareAstLiterals(ZM) \
    ZM(Void,     "void")       \
    ZM(Null,     "null")       \
    ZM(Char,     "char")       \
    ZM(Bool,     "bool")       \
    ZM(WChar,    "wchar")      \
    ZM(Utf8,     "utf8")       \
    ZM(SignedInt,"signed")     \
    ZM(UnsignedInt,"unsigned") \
    ZM(Float,    "float")      \
    ZM(Double,   "double")     \
    ZM(Text,     "text")
struct AstLiteral;

#define DeclareAstNames(ZM)    \
    ZM(Name,     "name")       \
    ZM(TypeName, "typename")
struct AstName;

#define DeclareAstNodes(ZM)     \
    ZM(Scope, "scope")          \
    /* Symbols */               \
    DeclareAstSymbols(ZM)       \
    /* Literals */              \
    DeclareAstLiterals(ZM)      \
    /* Names */                 \
    DeclareAstNames(ZM)

enum class AstKind {
#define ZM(zName, zText) zName,
    DeclareAstNodes(ZM)
#undef ZM
};

#define ZM(zName, zText) struct Ast##zName;
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
    using  Kind = AstKind;
    using  Node = AstNode*;
    using Nodes = List<Node>;
    using  Type = const AstType&;

    SourceLocation  loc;
    Kind            kind;
    AstType         type;

    AstNode(Loc loc, Kind kind, Type type) : loc(loc), kind(kind), type(type) {}
    virtual void dispose() {}
    String kindName() const;
    String kindValue() const;

    AstSymbol* isaSymbol();
    AstName* isaName();
};

struct AstScope : AstNode {
    using ParentScope = AstScope*;
    using ScopeOwner  = AstSymbol*;

    Dict<AstSymbol*> symbols;
    Nodes            nodes;
    ScopeOwner       owner;
    ParentScope      parent;

    AstScope(AstModule *owner);
    AstScope(ScopeOwner owner, ParentScope parent);
    void dispose() override;
    AstSymbol* find(Identifier);
    AstSymbol* findThroughDot(Identifier);
    Identifier name();
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
        void end() { Assert(isBusy()); value = Done; }
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

#define ZM(zName, zText) Ast##zName* isa##zName() { return kind == Kind::zName ? (Ast##zName*)this : nullptr; }
    DeclareAstSymbols(ZM)
#undef ZM
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
struct AstImport : AstSymbol {
    AstSymbol *symbol;

    AstImport(Loc loc, ParentScope parent, Identifier name, AstSymbol *symbol)
        : AstSymbol(loc, Kind::Import, symbol->type, parent, name), symbol(symbol) {}
};

struct AstExport : AstSymbol {
    AstSymbol *symbol;

    AstExport(Loc loc, ParentScope parent, Identifier name, AstSymbol *symbol)
        : AstSymbol(loc, Kind::Export, symbol->type, parent, name), symbol(symbol) {}
};
struct AstTypParam : AstSymbol {
    AstTypParam(Loc loc, Type type, ParentScope parent, Identifier name) 
        : AstSymbol(loc, Kind::TyParam, type, parent, name) {}
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
// Literals
struct AstLiteral : AstNode {
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
    AstLiteral(Loc loc, Kind kind, Type type, UINT64 u64) 
        : AstNode(loc, kind, type), u64(u64) {}
};

struct AstVoid : AstLiteral {
    AstVoid(Loc loc) 
        : AstLiteral(loc, Kind::Void, AstType(comp.ast->tyVoid), 0ui64) {}
};

struct AstNull : AstLiteral {
    AstNull(Loc loc) 
        : AstLiteral(loc, Kind::Null, comp.ast->tyVoid.pointer(), 0ui64) {}
};

struct AstChar : AstLiteral {
    AstChar(Loc loc, char ch) 
        : AstLiteral(loc, Kind::Char, comp.ast->tyChar, UINT64(UINT8(ch))) {}
};

struct AstBool : AstLiteral {
    AstBool(Loc loc, bool b) 
        : AstLiteral(loc, Kind::Bool, comp.ast->tyBool, UINT64(UINT8(b))) {}
};

struct AstWChar : AstLiteral {
    AstWChar(Loc loc, wchar_t ch) 
        : AstLiteral(loc, Kind::WChar, comp.ast->tyWChar, UINT64(UINT16(ch))) {}
};

struct AstUtf8 : AstLiteral {
    AstUtf8(Loc loc, UINT32 u32) 
        : AstLiteral(loc, Kind::Utf8, comp.ast->tyUtf8, UINT64(u32)) {}
};

struct AstSignedInt : AstLiteral {
    AstSignedInt(Loc loc, Type type, INT64 i64) 
        : AstLiteral(loc, Kind::SignedInt, type, UINT64(i64)) {}
};

struct AstUnsignedInt : AstLiteral {
    AstUnsignedInt(Loc loc, Type type, UINT64 u64) 
        : AstLiteral(loc, Kind::SignedInt, type, u64) {}
};

struct AstFloat : AstLiteral {
    AstFloat(Loc loc, float f32) 
        : AstLiteral(loc, Kind::Float, comp.ast->tyFloat, UINT64(meta::reinterpret<UINT32>(f32))) {}
};

struct AstDouble : AstLiteral {
    AstDouble(Loc loc, double f64) 
        : AstLiteral(loc, Kind::Double, comp.ast->tyDouble, meta::reinterpret<UINT64>(f64)) {}
};
//------------------------------------------------------------------------------------------------
// Names
struct AstName : AstNode {
    AstSymbol *symbol;

    AstName(Loc loc, AstSymbol *symbol) 
        : AstNode(loc, Kind::Name, symbol->type), symbol(symbol) {}
    AstName(Loc loc, Type type) 
        : AstNode(loc, Kind::TypeName, type) {}
};

struct AstTypeName : AstName {
    AstTypeName(Loc loc, Type type) 
        : AstName(loc, type) {}
};
} // namespace exy

#endif // AST_H