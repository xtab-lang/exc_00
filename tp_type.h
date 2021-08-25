#pragma once

namespace exy {
struct TpSymbol;
struct TpIndirectType;

struct TpType {
    enum class Kind {
        Unknown,
        Symbol,
        Pointer, Reference
    };

    TpType();
    TpType(TpSymbol*);
    TpType(TpIndirectType*, Kind);

    auto operator==(const TpType &other) const { return kind == other.kind && symbol == other.symbol; }
    auto operator!=(const TpType &other) const { return kind != other.kind || symbol != other.symbol; }

    auto isUnknown() const { return kind == Kind::Unknown; }
    auto isKnown()   const { return kind != Kind::Unknown; }

    auto isDirect()   const { return kind == Kind::Symbol ? symbol : nullptr; }
    auto isIndirect() const { return kind == Kind::Pointer || kind == Kind::Reference ? ptr : nullptr; }

    auto isaPointer()      const { return kind == Kind::Pointer   ? ptr : nullptr; }
    auto isaReference()    const { return kind == Kind::Reference ? ptr : nullptr; }
    auto isNotAPointer()   const { return kind != Kind::Pointer; }
    auto isNotAReference() const { return kind != Kind::Reference; }

    TpSymbol* isaBuiltin() const;
    auto isNotABuiltin() const { return isaBuiltin() == nullptr; }
    TpSymbol* isNumeric() const;  // Char ... Double
    auto isNotNumeric() const { return isNumeric() == nullptr; }
    TpSymbol* isIntegral() const; // Char ... UInt64
    auto isNotIntegral() const { return !isIntegral(); }
    TpSymbol* isSigned() const;   // Char ... Int64
    auto isNotSigned() const { return isSigned() == nullptr; }
    TpSymbol* isUnsigned() const; // Bool ... UInt64
    auto isNotUnsigned() const { return isUnsigned() == nullptr; }
    TpSymbol* isaFloatingPoint() const; // Float | Double
    auto isNotAFloatingPoint() const { return isaFloatingPoint() == nullptr; }
    TpSymbol* isPacked() const;  // Floatx4 ... UInt64x8
    auto isNotPacked() const { return isPacked() == nullptr; }
    TpSymbol* isPackedFloatingPoints() const; // Floatx4 | Doublex2 | Floatx8 | Doublex4 | Floatx16 | Doublex8
    auto isNotPackedFloatingPoints() const { return isPackedFloatingPoints() == nullptr; }
    TpSymbol* isPackedIntegrals() const; // Int8x16 ... UInt64x2 | Int8x32 ... UInt64x4 | Int8x64 ... UInt64x8
    auto isNotPackedIntegrals() const { return isPackedIntegrals() == nullptr; }
    TpSymbol* isFloatingPointOrPackedFloatingPoints() const; // Float | Double | Floatx4 | Doublex2 | Floatx8 | Doublex4 | Floatx16 | Doublex8
    auto isNotFloatingPointOrPackedFloatingPoints() const { return isFloatingPointOrPackedFloatingPoints() == nullptr; }
    TpSymbol* isIntegralOrPackedIntegrals() const; // Char ... UInt64 | Int8x16 ... UInt64x2 | Int8x32 ... UInt64x4 | Int8x64 ... UInt64x8
    auto isNotIntegralOrPackedIntegrals() const { return isIntegralOrPackedIntegrals() == nullptr; }
    bool isNumericOrIndirect() const; // Char ... Double | T* | T&
    auto isNotNumericOrIndirect() const { return !isNumericOrIndirect(); }

    TpSymbol* isaModule() const;
    TpSymbol* isOverloadSet() const;

    TpSymbol* isaTemplate() const;
    auto isNotATemplate() const { return isaTemplate() == nullptr; }

    TpSymbol* isaStruct() const;
    auto isNotAStruct() const { return isaStruct() == nullptr; }
    TpSymbol* isaStructTemplate() const;
    auto isNotAStructTemplate() const { return isaStructTemplate() == nullptr; }

    TpSymbol* isaUnion() const;
    auto isNotAUnion() const { return isaUnion() == nullptr; }
    TpSymbol* isaUnionTemplate() const;
    auto isNotAUnionTemplate() const { return isaUnionTemplate() == nullptr; }

    TpSymbol* isanEnum() const;
    auto isNotAnEnum() const { return isanEnum() == nullptr; }
    TpSymbol* isanEnumTemplate() const;
    auto isNotAnEnumTemplate() const { return isanEnumTemplate() == nullptr; }

    TpSymbol* isaStructOrUnionOrEnum() const;
    auto isNotAStructOrUnionOrEnum() const { return isaStructOrUnionOrEnum() == nullptr; }

    TpSymbol* isaFunction() const;
    auto isNotAFunction() const { return isaFunction() == nullptr; }
    TpSymbol* isaFunctionTemplate() const;
    auto isNotAFunctionTemplate() const { return isaFunctionTemplate() == nullptr; }
    TpSymbol* isaFunctionOrFunctionTemplate() const;
    auto isNotAFunctionOrFunctionTemplate() const { return isaFunctionOrFunctionTemplate() == nullptr; }

    auto isaBuiltinOrIndirect() const { return isaBuiltin() || isIndirect(); }
    auto isNotABuiltinOrIndirect() const { return !isaBuiltinOrIndirect(); }

    TpType mkPointer() const;
    TpType mkReference() const;
    TpType pointee() const;
    TpType dereference() const;
    TpType dereferenceIfReference() const;

    TpIndirectType* isVoidPointer() const;
    auto isNotVoidPointer() const { return isVoidPointer() == nullptr; }

#define ZM(zName, zSize) TpSymbol* is##zName() const;
    DeclareBuiltinTypeKeywords(ZM)
    #undef ZM

private:
    union {
        TpSymbol       *symbol;
        TpIndirectType *ptr;
    };
    Kind kind;
};

struct TpIndirectType {
    TpType pointee;

    TpIndirectType(const TpType &pointee) : pointee(pointee) {}
};

//----------------------------------------------------------
struct TpIndirectTypes {
    void initialize();
    void dispose();

    TpType pointerOf(const TpType&);
    TpType referenceOf(const TpType&);

private:
    List<TpIndirectType*> builtins{};
    Dict<TpIndirectType*, UINT64> ptrs{};
};
} // namespace exy