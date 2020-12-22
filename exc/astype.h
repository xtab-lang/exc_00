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
    struct AstBuiltin;
    struct AstModule;

struct AstType;
struct AstIndirectType;
    struct AstPointerType;
    struct AstReferenceType;
//----End forward declarations
struct AstType {
    enum class Kind { Direct, Pointer, Reference };

    AstType()                         : symbol(nullptr), kind(Kind::Direct)    {}
    AstType(AstSymbol        *symbol) : symbol(symbol),  kind(Kind::Direct)    {}
    AstType(AstPointerType   *ptr)    : ptr(ptr),        kind(Kind::Pointer)   {}
    AstType(AstReferenceType *ref)    : ref(ref),        kind(Kind::Reference) {}

    bool isUnknown() const { return !symbol; }
    bool isKnown()   const { return symbol;  }

    AstType pointer()   const;
    AstType reference() const;

    bool operator==(const AstType &other) const;
    bool operator!=(const AstType &other) const;

    AstSymbol*        isaSymbol()    const;
    AstSymbol*        isDirect()     const;
    bool              isIndirect()   const;
    AstPointerType*   isaPointer()   const;
    AstReferenceType* isaReference() const;

private:
    union {
        AstSymbol        *symbol;
        AstPointerType   *ptr;
        AstReferenceType *ref;
    };
    Kind kind;
};

struct AstIndirectType {
    AstType pointee;

    AstIndirectType(const AstType &pointee) : pointee(pointee) {}
};

struct AstPointerType : AstIndirectType {
    AstPointerType(const AstType &pointee) : AstIndirectType(pointee) {}
};

struct AstReferenceType : AstIndirectType {
    AstReferenceType(const AstType &pointee) : AstIndirectType(pointee) {}
};
} // namespace exy

#endif // AST_TYPE_H_