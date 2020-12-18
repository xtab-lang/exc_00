//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef AST_TYPE_H_
#define AST_TYPE_H_

namespace exy {
//--Begin forward declarations
struct AstSymbol;

struct AstType;
struct AstIndirectType;
    struct AstPointerType;
    struct AstReferenceType;
//----End forward declarations
struct AstType {
    enum class Kind { Direct, Indirect };

    AstType()                     : symbol(nullptr), kind(Kind::Direct) {}
    AstType(AstSymbol *symbol)    : symbol(symbol),  kind(Kind::Direct) {}
    AstType(AstIndirectType *ptr) : ptr(ptr),        kind(Kind::Indirect) {}

    bool isUnknown() const { return !symbol; }
    bool isKnown()   const { return symbol;  }

    AstType pointer()   const;
    AstType reference() const;

private:
    union {
        AstSymbol       *symbol;
        AstIndirectType *ptr;
    };
    Kind kind;
};

struct AstIndirectType {
    enum class Kind { Pointer, Reference };

    AstIndirectType(const AstType &pointee, Kind kind) : pointee(pointee), kind(kind) {}

private:
    AstType pointee;
    Kind    kind;
};

struct AstPointerType : AstIndirectType {
    AstPointerType(const AstType &pointee) : AstIndirectType(pointee, Kind::Pointer) {}
};

struct AstReferenceType : AstIndirectType {
    AstReferenceType(const AstType &pointee) : AstIndirectType(pointee, Kind::Reference) {}
};
} // namespace exy

#endif // AST_TYPE_H_