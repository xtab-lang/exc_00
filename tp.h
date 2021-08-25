#pragma once

#include "tp_type.h"

namespace exy {
struct SyntaxModule;
struct StructureSyntax;
struct FunctionSyntax;

/* Wraps a {TpSymbolNode}. */
struct TpSymbol;

/* Container for {TpSymbol}s. */
struct TpScope;

/* Base of all {TpSymbolNode}s. */
struct TpSymbolNode;

/* Base of all type {TpTypeNode}s. */
struct TpTypeNode;
struct TpArity;

#define DeclareTpTypeSymbolNodes(ZM)\
    /* Compiler knowns 🡓  */     \
    ZM(Builtin)                  \
    ZM(Module)                   \
    /* Templates 🡓  */           \
    ZM(OverloadSet)              \
    ZM(Template)                 \
    /* Structure instances 🡓  */ \
    ZM(Struct)                   \
    ZM(Union)                    \
    /* Fixed array instance 🡓 */ \
    ZM(Array)                    \
    /* Enumeration instance 🡓 */ \
    ZM(Enum)                     \
    /* Callables 🡓            */ \
    ZM(Function)                 \
    /* Labels 🡓               */ \
    ZM(Label)

/* Base of all type {TpAlias}s. */
struct TpAliasNode;

#define DeclareTpAliasSymbolNodes(ZM)\
    /* Aliases 🡓  */             \
    ZM(TypeAlias)                \
    ZM(ConstAlias)               \
    ZM(ValueAlias)

/* Base of all value {TpValueNode}s. */
struct TpValueNode;

#define DeclareTpValueSymbolNodes(ZM)  \
    /* Variables 🡓  */           \
    ZM(Parameter)                \
    ZM(Local)                    \
    ZM(Global)                   \
    ZM(Field)

#define DeclareTpSymbolNodes(ZM)  \
    DeclareTpTypeSymbolNodes(ZM)  \
    DeclareTpAliasSymbolNodes(ZM) \
    DeclareTpValueSymbolNodes(ZM)

// Base of all names.
struct TpName;

#define DeclareTpNameNodes(ZM) \
    ZM(TypeName)               \
    ZM(ValueName)              \
    ZM(FieldName)              \
    ZM(IndexName)              \
    ZM(ConstExpr)

#define DeclareTpStatementNodes(ZM)   \
    ZM(Block)                         \
    ZM(IfBlock)                       \
    ZM(Loop)                          \
    /* 'async+await+yield' magic 🡓 */ \
    ZM(Yield_)                        \
    ZM(YieldFrom)                     \
    /* Flow control                */ \
    ZM(Return)                        \
    ZM(Break)                         \
    ZM(Continue)                      \
    /* Others                      */ \
    ZM(Defer)

#define DeclareTpExpressionNodes(ZM)  \
    ZM(UnaryPrefix)                   \
    ZM(UnarySuffix)                   \
    ZM(PointerUnaryPrefix)            \
    ZM(PointerUnarySuffix)            \
    ZM(Definition)                    \
    ZM(Assignment)                    \
    ZM(CompoundPointerArithmetic)     \
    ZM(CompoundShift)                 \
    ZM(CompoundArithmetic)            \
    ZM(PointerArithmetic)             \
    ZM(Shift)                         \
    ZM(Arithmetic)                    \
    ZM(Condition)                     \
    ZM(Ternary)                       \
    ZM(Call)                          \
    ZM(Initializer)                   \
    /* 'async+await+yield' magic 🡓 */ \
    ZM(Await)                         \
    /* Memory operations 🡓         */ \
    ZM(SizeOf)                        \
    ZM(New)                           \
    ZM(Delete)                        \
    ZM(Atomic)                        \
    ZM(AddressOf)                     \
    ZM(ReferenceOf)                   \
    ZM(Dereference)                   \
    /* Names 🡓                     */ \
    DeclareTpNameNodes(ZM)            \
    ZM(Cast)                          \
    ZM(Literal)

#define DeclareTpNodes(ZM)      \
    DeclareTpSymbolNodes(ZM)    \
    DeclareTpStatementNodes(ZM) \
    DeclareTpExpressionNodes(ZM)

#define ZM(zName) struct Tp##zName;
DeclareTpNodes(ZM)
#undef ZM

//----------------------------------------------------------
enum class TpKind {
    Unknown,
#define ZM(zName) zName,
    DeclareTpNodes(ZM)
#undef ZM
};
//----------------------------------------------------------
 struct TpTree {
     Mem      mem;   // The 1 and only {Mem} allocator for {this} tree.
     TpScope *scope; // The root {TpScope} of {this} tree. Contains {TpModule}s, {TpBuiltin}s and 'urlhandler' {TpTemplate}s.
     List<TpSymbol*> modules; // All the {TpModule}s in {this} tree.

     static TpType tyUnknown;
     static TpType tyVoidPointer;
 #define ZM(zName, zSize) static TpType ty##zName;
     DeclareBuiltinTypeKeywords(ZM)
 #undef ZM

     bool initialize();
     void dispose();
 private:
     void initializeBuiltins();
     bool initializeModules();
     void initializeModule(TpModule *node);
 };
 //----------------------------------------------------------
 struct TpSymbol {
     using ParentScope = TpScope*;

     ParentScope   scope;
     Identifier    name;
     TpSymbolNode *node;
     Status        bindStatus;

     TpSymbol(ParentScope scope, Identifier name, TpSymbolNode *node);
     void dispose();
 };
 //----------------------------------------------------------
 struct TpScope {
     TpScope         *parent; // The immediate parent {TpScope} of {this} scope.
     TpSymbol        *owner;  // The actual owner {TpSymbol} of {this} scope.
     Dict<TpSymbol*> symbols; // All the {TpSymbols} declared in {this} scope.
     List<TpNode*>   statements;

     TpScope(TpScope *parent, TpSymbol *owner);
     void dispose();

     TpSymbol* contains(Identifier name);
     TpSymbol* append(TpSymbol*);
     template<typename T>
     T* append(T *statement) {
         if (statement != nullptr) statements.append(statement);
         return statement;
     }
 };
//----------------------------------------------------------
struct TpNode {
    using  Pos = const SourcePos&;
    using Kind = TpKind;
    using Type = const TpType&;

    SourcePos pos;
    TpType    type;
    Kind      kind;

    TpNode(Pos pos, Type type, Kind kind);
    virtual void dispose() {}

    String kindName() const;
    static String kindName(Kind);
};
//----------------------------------------------------------
struct TpModifiers {
    /* Visibility modifiers  */
    UINT   isPrivate : 1;
    UINT  isInternal : 1;
    UINT isProtected : 1;
    /* Scope modifiers  */
    UINT isStatic : 1;
    /* Mutability modifiers  */
    UINT    isConst : 1;
    UINT isReadOnly : 1;
    /* Lifetime modifiers  */
    UINT isAuto : 1;
    /* Info modifiers  */
    UINT isVar : 1;
    /* Async modifiers  */
    UINT isAsync : 1;
    /* Heirarchy modifiers  */
    UINT isAbstract : 1;
    UINT isOverride : 1;
    /* Synchronization modifiers  */
    UINT isSynchronized : 1;
    /* Generated modifiers  */
    UINT isaGenerator : 1;

    auto isResumable() { return isAsync || isaGenerator; }
};

struct TpSymbolNode : TpNode {
    Identifier  dotName;
    TpModifiers modifiers;
    bool isCompilerGenerated;

    TpSymbolNode(Pos pos, Type type, Kind kind, Identifier dotName);
};

//----------------------------------------------------------
struct TpTypeNode : TpSymbolNode {
    using OwnScope = TpScope*;

    OwnScope scope;

    TpTypeNode(Pos pos, Kind kind, Identifier dotName);
    void dispose() override;
};

struct TpPacking {
    TpType type;
    INT    count;

    TpType packedType() const;
};

struct TpBuiltin : TpTypeNode {
    INT     size;
    Keyword keyword;

    TpBuiltin(Pos pos, Keyword keyword, Identifier dotName);
    TpPacking packing() const;
};

struct TpModule : TpTypeNode {
    List<TpSymbol*> urlHandlers; // List of 'urlhandler' functions declared in {this} module.
    SyntaxModule   *syntax;      // Source syntax for {this} module.
    Identifier      system;      // 'console' ... 'dll'
    TpSymbol       *main;        // The 1 and only 'main' function of {this} module.

    TpModule(SyntaxModule *syntax);
    void dispose() override;
};

struct TpOverloadSet : TpTypeNode {
    List<TpSymbol*> list;

    TpOverloadSet(TpSymbol *first);
    void dispose() override;
};

struct TpArity {
    INT required;
    INT defaults;
    bool varags;
    bool hasThis;

    auto isZero()    const { return required == 0 && defaults == 0 && !varags; }
    auto isNotZero() const { return required > 0 || defaults > 0 || varags; }
};

struct TpTemplate : TpTypeNode {

    List<TpSymbol*> instances;
    TpOverloadSet  *parentOv;
    SyntaxNode     *syntax;
    Identifier      dllPath;
    TpArity         arity;

    TpTemplate(StructureSyntax *syntax, const TpArity &parameters, Identifier dotName);
    TpTemplate(FunctionSyntax *syntax, const TpArity &parameters, Identifier dotName);
    void dispose() override;
};

struct TpStruct : TpTypeNode {
    enum StructKind {
        OrdinaryStruct,  // {this} is a user-defined struct with the 'struct' keyword.
        TupleStruct,     // {this} is a user-defined struct without the 'struct' keyword.
        ResumableStruct, // {this} is a compiler generated struct for 'async+await' magic!
        LambdaStruct,    // {this} is a compiler generated struct for 'lambda' magic!
    };
    List<TpSymbol*> bases;
    List<TpSymbol*> derived;
    List<TpSymbol*> parameters;
    StructKind      structKind;

    TpStruct(Pos pos, Identifier dotName, StructKind structKind);
    void dispose() override;

    auto isaTupleStruct()     const { return structKind == TupleStruct; }
    auto isaResumableStruct() const { return structKind == ResumableStruct; }
    auto isaLambdaStruct()    const { return structKind == LambdaStruct; }
};

struct TpUnion : TpTypeNode {
    TpUnion(Pos pos, Identifier dotName);
};

struct TpArray : TpTypeNode {
    INT length;

    TpArray(Pos pos, Identifier dotName, INT length);
};

struct TpEnum : TpTypeNode {
    TpType valueType;

    TpEnum(Pos pos, Identifier dotName);
};

struct TpFunction : TpTypeNode {
    List<TpSymbol*> parameters;
    TpType          fnreturn;
    Identifier      dllPath;
    Keyword         keyword;

    TpFunction(Pos pos, Keyword keyword, Identifier dotName);
    void dispose() override;

    TpSymbol* isaConstructor() const;
    TpSymbol* isaDisposeFunction() const;
    TpSymbol* isaGeneratorFunction() const;
    TpSymbol* isaLambdaFunction() const;
};

struct TpLabel : TpTypeNode {
    TpBlock *block;

    TpLabel(Pos pos, Identifier dotName);
};
//----------------------------------------------------------
enum class TpAliasKind { Define, Import, Export };

struct TpAliasNode : TpSymbolNode {
    TpAliasKind aliasKind;

    TpAliasNode(Pos pos, Type type, Kind kind, Identifier dotName, TpAliasKind aliasKind);
};

struct TpTypeAlias : TpAliasNode {
    TpTypeAlias(Pos pos, Type type, Identifier dotName, TpAliasKind aliasKind);
};

struct TpConstAlias : TpAliasNode {
    TpConstExpr *value;

    TpConstAlias(Pos pos, TpConstExpr *value, Identifier dotName, TpAliasKind aliasKind);
    void dispose() override;
};

struct TpValueAlias : TpAliasNode {
    TpSymbol *value;

    TpValueAlias(Pos pos, TpSymbol *value, Identifier dotName, TpAliasKind aliasKind);
};

//----------------------------------------------------------
struct TpValueNode: TpSymbolNode {
    TpValueNode(Pos pos, Type type, Kind kind, Identifier dotName);
};

struct TpParameter : TpValueNode {
    TpParameter(Pos pos, Type type, Identifier dotName);
};

struct TpLocal : TpValueNode {
    TpLocal(Pos pos, Type type, Identifier dotName);
};

struct TpGlobal : TpValueNode {
    TpNode *rhs;

    TpGlobal(Pos pos, Type type, Identifier dotName);
    TpGlobal(Pos pos, TpNode *rhs, Identifier dotName);
    void dispose() override;
};

struct TpField : TpValueNode {
    enum FieldKind { OrdinaryField, CaptureField, TypeIdField, SuperField };

    TpNode   *rhs;
    FieldKind fieldKind;

    TpField(Pos pos, Type type, Identifier dotName, FieldKind);
    TpField(Pos pos, TpNode *rhs, Identifier dotName, FieldKind);
    void dispose() override;
};

//----------------------------------------------------------
// Statements.
struct TpBlock : TpNode {
    TpScope  *scope;

    TpBlock(Pos pos, TpLabel *label);
    TpBlock(Pos pos, TpLabel *label, Kind kind);
    void dispose() override;
};

struct TpIfBlock : TpBlock {
    TpNode  *condition;
    TpBlock *ifalse;

    TpIfBlock(Pos pos, TpLabel *label);
    void dispose() override;
};

struct TpLoop : TpBlock {
    TpIfBlock *body;
    TpBlock   *ifnobreak;

    TpLoop(Pos pos, TpLabel *label);
    void dispose() override;
};

struct TpYield : TpNode {
    TpNode *value;

    TpYield(Pos pos, TpNode *value);
    void dispose() override;
};

struct TpYieldFrom : TpNode {
    TpNode *value;

    TpYieldFrom(Pos pos, TpNode *value);
    void dispose() override;
};

struct TpReturn : TpNode {
    TpNode *value;

    TpReturn(Pos pos);
    TpReturn(Pos pos, TpNode *value);
    void dispose() override;
};

struct TpBreak : TpNode {
    TpBreak(Pos pos);
};

struct TpContinue : TpNode {
    TpContinue(Pos pos);
};

struct TpDefer : TpNode {
    TpNode *value;

    TpDefer(Pos pos, TpNode *value);
    void dispose() override;
};

//----------------------------------------------------------
// Expressions.
struct TpUnary : TpNode {
    TpNode *value;
    Tok     op;

    TpUnary(Pos pos, TpNode *value, Tok op, Kind kind);
    void dispose() override;
};

struct TpUnaryPrefix : TpUnary {
    TpUnaryPrefix(Pos pos, Tok op, TpNode *value);
};

struct TpUnarySuffix : TpUnary {
    TpUnarySuffix(Pos pos, TpNode *value, Tok op);
};

struct TpPointerUnaryPrefix : TpUnary {
    TpPointerUnaryPrefix(Pos pos, Tok op, TpNode *value);
};

struct TpPointerUnarySuffix : TpUnary {
    TpPointerUnarySuffix(Pos pos, TpNode *value, Tok op);
};

struct TpDefinition : TpNode {
    TpNode *lhs;
    TpNode *rhs;

    TpDefinition(Pos pos, TpNode *lhs, TpNode *rhs);
    void dispose() override;
};

struct TpAssignment : TpNode {
    TpNode *dst;
    TpNode *src;
    Tok     op;

    TpAssignment(Pos pos, TpNode *dst, TpNode *src);
    TpAssignment(Pos pos, TpNode *dst, Tok op, TpNode *src, TpKind kind);
    void dispose() override;
};

//    T* '-=' U/Int64
// or T* '+=' U/Int64
struct TpCompoundPointerArithmetic : TpAssignment {
    TpCompoundPointerArithmetic(Pos pos, TpNode *dst, Tok op, TpNode *src);
};

// '<<=', '>>=' or '>>>='
struct TpCompoundShift : TpAssignment {
    TpCompoundShift(Pos pos, TpNode *dst, Tok op, TpNode *src);
};

//        COMPOUND_BITWISE_OP: '|=', '^=' or '&='
//       COMPOUND_ADDITIVE_OP: '-=' or '+='
// COMPOUND_MULTIPLICATIVE_OP: '*=', '/=', '%=', '%%=' or '**='
struct TpCompoundArithmetic : TpAssignment {
    TpCompoundArithmetic(Pos pos, TpNode *dst, Tok op, TpNode *src);
};

struct TpBinary : TpNode {
    TpNode *lhs;
    TpNode *rhs;
    Tok     op;

    TpBinary(Pos pos, Type type, TpNode *lhs, Tok op, TpNode *rhs, TpKind kind);
    void dispose() override;
};

//    T* '-' U/Int64 → T*
// or U/Int64 '-' T* → T*
// or T* '-' T*      → Int64
// or T* '+' U/Int64 → T*
struct TpPointerArithmetic : TpBinary {
    TpPointerArithmetic(Pos pos, Type type, TpNode *lhs, Tok op, TpNode *rhs);
};

// '<<', '>>' or '>>>'
struct TpShift : TpBinary {
    TpShift(Pos pos, TpNode *lhs, Tok op, TpNode *rhs);
};

//        BITWISE_OP: '|', '^' or '&'
//       ADDITIVE_OP: '-' or '+'
// MULTIPLICATIVE_OP: '*', '/', '%', '%%' or '**'
struct TpArithmetic : TpBinary {
    TpArithmetic(Pos pos, TpNode *lhs, Tok op, TpNode *rhs);
};

//     LOGIAL_OP: '||' or '&&'
// RELATIONAL_OP: '!==', '!=', '===', '==', '<', '<=', '>' or '>='
struct TpCondition : TpBinary {
    TpCondition(Pos pos, TpNode *lhs, Tok op, TpNode *rhs);
};

struct TpTernary : TpNode {
    TpNode *condition;
    TpNode *iftrue;
    TpNode *ifalse;

    TpTernary(Pos pos, TpNode *condition, TpNode *iftrue, TpNode *ifalse);
    void dispose() override;
};

struct TpCall : TpNode {
    TpNode       *name;
    List<TpNode*> arguments;

    TpCall(Pos pos, Type type, TpNode *name);
    void dispose() override;
};

struct TpInitializer : TpNode {
    List<TpNode*> arguments;

    TpInitializer(Pos pos, Type type);
    void dispose() override;
};

//----------------------------------------------------------
// Expressions for generator functions.
struct TpAwait : TpNode {
    TpNode *value;

    TpAwait(Pos pos, Type type, TpNode *value);
    void dispose() override;
};

//----------------------------------------------------------
// Expressions for memory operations.
struct TpSizeOf : TpNode {
    TpType value;

    TpSizeOf(Pos pos, Type value);
};

struct TpNew : TpNode {
    TpNew(Pos pos, Type type);
    void dispose() override;
};

struct TpDelete : TpNode {
    TpNode *value;

    TpDelete(Pos pos, TpNode *value);
    void dispose() override;
};

struct TpAtomic : TpNode {
    TpNode *value;

    TpAtomic(Pos pos, TpNode *value);
    void dispose() override;
};

struct TpAddressOf : TpNode {
    TpNode *value;

    TpAddressOf(Pos pos, TpNode *value);
    void dispose() override;
};

struct TpReferenceOf : TpNode {
    TpNode *value;

    TpReferenceOf(Pos pos, TpNode *value);
    void dispose() override;
};

struct TpDereference : TpNode {
    TpNode *value;

    TpDereference(Pos pos, TpNode *value);
    void dispose() override;
};

//----------------------------------------------------------
// Expressions for names
struct TpName : TpNode {
    TpName(Pos pos, Type type, Kind kind);
};

struct TpTypeName : TpName {
    TpTypeName(Pos pos, Type type);
};

struct TpValueName : TpName {
    TpSymbol *symbol; // Global | Local | Parameter

    TpValueName(Pos pos, TpSymbol *symbol);
};

struct TpFieldName : TpName {
    TpNode   *base;
    TpSymbol *symbol; // Field

    TpFieldName(Pos pos, TpNode *base, TpSymbol *symbol);
    void dispose() override;
};

struct TpIndexName : TpName {
    TpNode *base;
    TpNode *index; // Int64 | UInt64

    TpIndexName(Pos pos, Type type, TpNode *base, TpNode *index);
    void dispose() override;
};

struct TpConstExpr : TpName {
    TpNode *value; // Literal | expression with ConstExpr

    TpConstExpr(Pos pos, TpNode *value);
};

struct TpCast : TpNode {
    enum Reason { ExplicitCast, ImplicitCast };
    enum CastKind {
        NoCast,

        FromNullCast, // null ⟶ U ; where U is any type

        FloatCast,      // T ⟶ U ; where either T or U is a floating point
        WideningCast,   // T ⟶ U ; where both T and U are numeric builtins of the same sign and U is larger than T
        NarrowingCast,  // T ⟶ U ; where both T and U are numeric builtins of the same sign and U is smaller than T
        BitCast,        // T ⟶ U ; where both T and U are of the same bit-size and it is a bit-by-bit reinterpretation
        MemberWiseCast, // T ⟶ U ; where each member is cast independently

        PackedFloatCast,
        PackedBitCast,
        PackedBroadCast, // T ⟶ Packed

        _end_casts,
                
        AddressOfCast,   // T ⟶ T* ; 'addressOf' operator
        ReferenceOfCast, // T ⟶ T& ; 'referenceOf' operator
        DereferenceCast, // T& ⟶ T or T* ⟶ T; 'dereference' operator
        ToBoolCast,      // T != zeroOf(T)
    };
    TpNode  *value;
    CastKind castKind;
    Reason   reason;

    TpCast(Pos pos, Type type, TpNode *value, CastKind castKind, Reason reason);
    void dispose() override;
};

//----------------------------------------------------------
struct TpLiteral : TpNode {
    union {
        bool   b;
        UINT8  u8;
        CHAR   ch;
        INT8   i8;
        UINT16 u16;
        WCHAR  wch;
        INT16  i16;
        UINT32 u32;
        INT32  i32;
        UINT64 u64;
        INT64  i64;
        FLOAT  f32;
        DOUBLE f64;
    };
    TpLiteral(Pos pos, Type type, UINT64 u64);
};
} // namespace exy