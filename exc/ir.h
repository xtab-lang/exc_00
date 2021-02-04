//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-23
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef IR_H
#define IR_H

#include "register.h"
#include "buffer.h"

namespace exy {
//--Begin forward declarations
struct IrNode;

struct IrSection;
#define DeclarePeSections(ZM)   \
    ZM(CodeSection)             \
    ZM(DataSection)             \
    ZM(ImportSection)           \
    ZM(ExportSection)           \
    ZM(StringTable)

struct IrTypeSymbol;
#define DeclareIrTypeSymbols(ZM)    \
    DeclarePeSections(ZM)           \
    ZM(Builtin)                     \
    ZM(Module)                      \
    ZM(Function)                    \
    ZM(StackFrame)                  \
    ZM(Block)

#define DeclareIrValueSymbols(ZM)   \
    ZM(Global)
struct IrValueSymbol;

#define DeclareIrSymbols(ZM)    \
    DeclareIrTypeSymbols(ZM)    \
    DeclareIrValueSymbols(ZM)
struct IrSymbol;

struct IrUse;
#define DeclareIrOperations(ZM) \
    ZM(Exit)                    \
    ZM(Constant)                \
    ZM(Condition)               \
    ZM(Jump)                    \
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
    Loc loc() const;
};

struct IrNode {
    using Kind = IrKind;
    using Type = const IrType&;

    SourceLocation   loc;
    IrType           type;
    Kind             kind;
    List<IrUse*>     uses;
    Register         reg;
    int              idx;

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

struct IrSection : IrTypeSymbol {
    IrModule *parentModule;
    Buffer    peBuffer;
    Buffer    txtBuffer;

    IrSection(IrModule *parentModule, Kind kind, Identifier name);
    void dispose() override;
};

struct IrCodeSection : IrSection {
    List<IrFunction*> functions;

    IrCodeSection(IrModule *parentModule)
        : IrSection(parentModule, Kind::CodeSection, ids.dot.text) {}
    void dispose() override;
};

struct IrDataSection : IrSection {
    List<IrBuiltin*> builtins;
    List<IrGlobal*>  globals;

    IrDataSection(IrModule *parentModule)
        : IrSection(parentModule, Kind::DataSection, ids.dot.data) {}
    void dispose() override;
};

struct IrImportSection : IrSection {
    IrImportSection(IrModule *parentModule)
        : IrSection(parentModule, Kind::ImportSection, ids.dot.idata) {}
};

struct IrExportSection : IrSection {
    IrExportSection(IrModule *parentModule)
        : IrSection(parentModule, Kind::ExportSection, ids.dot.edata) {}
};

struct IrStringTable : IrSection {
    IrStringTable(IrModule *parentModule)
        : IrSection(parentModule, Kind::StringTable, ids.dot.string) {}
};

struct IrBuiltin : IrTypeSymbol {
    Keyword keyword;

    IrBuiltin(Loc loc, IrDataSection *parentSection, Identifier name, Keyword keyword)
        : IrTypeSymbol(loc, Kind::Builtin, name), keyword(keyword) {
        parentSection->builtins.append(this);
    }
};

struct IrModule : IrTypeSymbol {
    IrCodeSection   *code;
    IrDataSection   *data;
    IrImportSection *imports;
    IrExportSection *exports;
    IrStringTable   *strings;
    IrFunction      *entry;
    BinaryKind       binaryKind;

    IrModule(Loc loc, Identifier name, BinaryKind binaryKind);
    void dispose() override;

    bool isaDll();
    bool isExecutable();
};

struct IrFunction : IrTypeSymbol {
    IrCodeSection  *parentSection;
    IrStackFrame   *stack;
    IrType          retype;
    Queue<IrBlock>  body;

    IrFunction(Loc loc, IrCodeSection *parentSection, Identifier name, Type retype);
    void dispose() override;
};

struct IrStackFrame : IrTypeSymbol {
    IrStackFrame(IrFunction *parentFunction)
        : IrTypeSymbol(parentFunction->loc, Kind::StackFrame, ids.dot.stack) {}
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
    IrDataSection *parentSection;

    IrGlobal(Loc loc, IrDataSection *parentSection, Type type, Identifier name)
        : IrValueSymbol(loc, Kind::Global, type, name), parentSection(parentSection) {
        parentSection->globals.append(this);
    }
};
//------------------------------------------------------------------------------------------------
enum class IrUseKind {
    Read, Write
};
struct IrUse {
    IrOperation *user;
    IrNode      *value;
    IrUseKind    useKind;
    Register     reg;

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

struct IrExit : IrOperation {
    IrExit(Loc loc) : IrOperation(loc, Kind::Exit, comp.ir->tyVoid) {}
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

#define DeclareIrConditionOps(ZM)   \
    ZM(Equal,    "==")              \
    ZM(NotEqual, "!=")
/*
    flag ← lhs OP rhs
*/
struct IrCondition : IrOperation {
    enum class Op {
    #define ZM(zName, zText) zName,
        DeclareIrConditionOps(ZM)
    #undef ZM
    };

    IrUse lhs;
    IrUse rhs;
    Op    op;

    IrCondition(Loc loc, IrNode *lhs, Op op, IrNode *rhs)
        : IrOperation(loc, Kind::Condition, comp.ir->tyBool), lhs(this, lhs, IrUseKind::Read), op(op),
        rhs(this, rhs, IrUseKind::Read) {}
};
/*
        flag ← lhs OP rhs
        goto iftrue if flag
    or
        goto iftrue
*/
struct IrJump : IrOperation {
    IrUse condition;
    IrUse iftrue;

    IrJump(Loc loc, IrNode *iftrue)
        : IrOperation(loc, Kind::Jump, comp.ir->tyVoid), condition(this, nullptr, IrUseKind::Read),
        iftrue(this, iftrue, IrUseKind::Read) {}

    IrJump(Loc loc, IrCondition *condition, IrNode *iftrue)
        : IrOperation(loc, Kind::Jump, comp.ir->tyVoid), condition(this, condition, IrUseKind::Read),
        iftrue(this, iftrue, IrUseKind::Read) {}

    bool isConditional() { return condition.value != nullptr; }
    bool isUnconditional() { return condition.value == nullptr; }
};

/*
    [base + index × scale + disp]
*/
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
    xmm ← gpr ← imm
    xmm ← [mem]

    [mem] ← gpr
    [mem] ← xmm
    [mem] ← imm
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

struct ModuleProvider : WorkProvider<IrModule> {
    int pos{};

    bool next(List<IrModule*> &batch);
};
} // namespace exy

#endif // IR_H