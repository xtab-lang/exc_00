//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-23
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef IR_H
#define IR_H

namespace exy {
//--Begin forward declarations
struct IrNode;

#define DeclareIrTypeSymbols(ZM)    \
    ZM(Builtin)                     \
    ZM(Module)                      \
    ZM(Function)                    \
    ZM(Block)
struct IrTypeSymbol;

#define DeclareIrValueSymbols(ZM)   \
    ZM(Global)
struct IrValueSymbol;

#define DeclareIrSymbols(ZM)    \
    DeclareIrTypeSymbols(ZM)    \
    DeclareIrValueSymbols(ZM)
struct IrSymbol;

struct IrUse;
#define DeclareIrOperations(ZM) \
    ZM(Condition)               \
    ZM(Jump)                    \
    ZM(Constant)                \
    ZM(Path)                    \
    ZM(Assign)                  \
    ZM(Cast)
struct IrOperation;

#define DeclareIrNodes(ZM)      \
    /* Symbols */               \
    DeclareIrSymbols(ZM)        \
    /* Operations */            \
    DeclareIrOperations(ZM)

enum class IrKind {
#define ZM(zName) zName,
    DeclareIrNodes(ZM)
#undef ZM
};

#define ZM(zName) struct Ir##zName;
    DeclareIrNodes(ZM)
#undef ZM

//----End forward declarations

struct IrType {
    IrType() : symbol(nullptr), ptr(0) {}
    IrType(IrTypeSymbol *symbol) : symbol(symbol), ptr(0) {}
    IrType(IrTypeSymbol *symbol, int ptr) : symbol(symbol), ptr(ptr) {}

    bool isValid() const { return symbol != nullptr; }
    bool operator==(const IrType &other) const { return symbol == other.symbol && ptr == other.ptr; }
    bool operator!=(const IrType &other) const { return symbol != other.symbol || ptr != other.ptr; }
    IrType pointer() const;

    int isaPointer() const { return ptr; }
    IrType pointee() const;
    IrTypeSymbol* isDirect() const;
private:
    IrTypeSymbol *symbol;
    int           ptr;
};

struct IrTree {
    Mem              mem;
    List<IrModule*>  modules;   // First module is the application's entry.

#define ZM(zName, zSize) static IrType ty##zName;
    DeclareBuiltinTypeKeywords(ZM)
 #undef ZM
    IrTree();
    void dispose();
};

struct IrNode {
    using Kind = IrKind;
    using Type = const IrType&;

    SourceLocation   loc;
    IrType           type;
    Kind             kind;
    List<IrUse*>     uses;

    IrNode(Loc loc, Kind kind, Type type) 
        : loc(loc), kind(kind), type(type) {}
    virtual void dispose();
    String kindName();
    static String kindName(Kind);
};
//------------------------------------------------------------------------------------------------
struct IrSymbol : IrNode {
    Identifier name;

    IrSymbol(Loc loc, Kind kind, Type type, Identifier name) 
        : IrNode(loc, kind, type), name(name) {}
};
//------------------------------------------------------------------------------------------------
// Type symbols.
struct IrTypeSymbol : IrSymbol {
    IrTypeSymbol(Loc loc, Kind kind, Identifier name) : IrSymbol(loc, kind, IrType(this), name) {}
};

struct IrBuiltin : IrTypeSymbol {
    Keyword keyword;

    IrBuiltin(Loc loc, Identifier name, Keyword keyword)
        : IrTypeSymbol(loc, Kind::Builtin, name), keyword(keyword) {}
};

struct IrModule : IrTypeSymbol {
    List<IrSymbol*> symbols;
    IrFunction     *entry;

    IrModule(Loc loc, Identifier name)
        : IrTypeSymbol(loc, Kind::Module, name) {
    }
    void dispose() override;
};

struct IrFunction : IrTypeSymbol {
    IrType          retype;
    Queue<IrBlock>  body;

    IrFunction(Loc loc, Identifier name, Type retype) 
        : IrTypeSymbol(loc, Kind::Function, name), retype(retype) {}
    void dispose() override;
};

struct IrBlock : IrTypeSymbol {
    IrBlock           *qnext;
    IrBlock           *qprev;
    Queue<IrOperation> body;

    IrBlock(Loc loc, Identifier name) 
        : IrTypeSymbol(loc, Kind::Block, name) {}
    void dispose() override;

    template<typename T>
    T* append(T *operation) {
        return (T*)body.append(operation);
    }
};
//------------------------------------------------------------------------------------------------
struct IrValueSymbol : IrSymbol {
    IrValueSymbol(Loc loc, Kind kind, Type type, Identifier name)
        : IrSymbol(loc, kind, type, name) {}
};

struct IrGlobal : IrValueSymbol {
    IrGlobal(Loc loc, Type type, Identifier name)
        : IrValueSymbol(loc, Kind::Global, type, name) {}
};
//------------------------------------------------------------------------------------------------
enum class IrUseKind {
    Read, Write, Both
};
struct IrUse {
    IrOperation *user;
    IrNode      *value;
    IrUseKind    useKind;

    IrUse(IrOperation *user, IrNode *value, IrUseKind useKind);
};
//------------------------------------------------------------------------------------------------
struct IrOperation : IrNode {
    IrOperation *qnext;
    IrOperation *qprev;
    int          index;

    IrOperation(Loc loc, Kind kind, Type type) 
        : IrNode(loc, kind, type) {}
};

#define DeclareIrConditionOps(ZM)   \
    ZM(Equal,   "==")               \
    ZM(NotEqual, "!=")

struct IrCondition : IrOperation {
    enum class Op {
    #define ZM(zName, zText) zName,
        DeclareIrConditionOps(ZM)
    #undef ZM
    };

    IrUse lhs;
    IrUse rhs;
    Op       op;

    IrCondition(Loc loc, IrNode *lhs, Op op, IrNode *rhs) 
        : IrOperation(loc, Kind::Condition, comp.ir->tyBool), lhs(this, lhs, IrUseKind::Read), op(op), 
        rhs(this, rhs, IrUseKind::Read) {}
};

struct IrJump : IrOperation {
    IrUse condition;
    IrUse iftrue;

    IrJump(Loc loc, IrNode *iftrue) 
        : IrOperation(loc, Kind::Jump, comp.ir->tyVoid), condition(this, nullptr, IrUseKind::Read), 
        iftrue(this, iftrue, IrUseKind::Read) {}

    IrJump(Loc loc, IrCondition *condition, IrNode *iftrue) 
        : IrOperation(loc, Kind::Jump, comp.ir->tyVoid), condition(this, condition, IrUseKind::Read), 
        iftrue(this, iftrue, IrUseKind::Read) {}

    bool isConditional()   { return condition.value != nullptr; }
    bool isUnconditional() { return condition.value == nullptr; }
};

struct IrConstant : IrOperation {
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
    IrConstant(Loc loc, Type type, UINT64 u64) 
        : IrOperation(loc, Kind::Constant, type), u64(u64) {}
};

struct IrPath : IrOperation {
    IrUse base;
    IrUse index;

    IrPath(Loc loc, IrNode *base, IrNode *index) 
        : IrOperation(loc, Kind::Cast, index->type), base(this, base, IrUseKind::Read), 
        index(this, index, IrUseKind::Read) {}
};

/*
    gpr ← gpr
    gpr ← xmm
    gpr ← imm
    gpr ← [mem]

    xmm ← gpr
    xmm ← xmm
    xmm ← [mem]

    [mem] ← gpr
    [mem] ← xmm

*/
struct IrAssign : IrOperation {
    IrUse lhs;
    IrUse rhs;

    IrAssign(Loc loc, IrNode *lhs, IrNode *rhs) 
        : IrOperation(loc, Kind::Assign, lhs->type), lhs(this, lhs, IrUseKind::Write), 
        rhs(this, rhs, IrUseKind::Read) {}
};

struct IrCast : IrOperation {
    IrUse src;

    IrCast(Loc loc, Type type, IrNode *src) 
        : IrOperation(loc, Kind::Cast, type), src(this, src, IrUseKind::Read) {}
};

} // namespace exy

#endif // IR_H