//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-23
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ir.h"

namespace exy {
//------------------------------------------------------------------------------------------------
IrType IrType::pointer() const {
    return { symbol, ptr + 1 };
}

IrType IrType::pointee() const {
    if (ptr) {
        return { symbol, ptr - 1 };
    }
    Unreachable();
}

IrTypeSymbol* IrType::isDirect() const {
    return ptr ? nullptr : symbol;
}
//------------------------------------------------------------------------------------------------
IrTree::IrTree() {
    comp.ir = this;
}
void IrTree::dispose() {
    ldispose(modules);
    mem.dispose();
}
Loc IrTree::loc() const {
    return modules.first()->loc;
}
//------------------------------------------------------------------------------------------------
void IrNode::dispose() {
    uses.dispose();
}

String IrNode::kindName() {
    return kindName(kind);
}

String IrNode::kindName(Kind k) {
    switch (k) {
    #define ZM(zName) case Kind::zName: return { S(#zName), 0u };
        DeclareIrNodes(ZM)
        #undef ZM
    }
    Unreachable();
}
//------------------------------------------------------------------------------------------------
IrSection::IrSection(IrModule *parentModule, Kind kind, Identifier name) 
    : IrTypeSymbol(parentModule->loc, kind, name), parentModule(parentModule) {}

void IrSection::dispose() {
    peBuffer.dispose();
    txtBuffer.dispose();
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
void IrCodeSection::dispose() {
    ldispose(functions);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
void IrDataSection::dispose() {
    ldispose(builtins);
    ldispose(globals);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
IrModule::IrModule(Loc loc, Identifier name, BinaryKind binaryKind)
    : IrTypeSymbol(loc, Kind::Module, name), binaryKind(binaryKind) {
    auto &mem = comp.ir->mem;
    comp.ir->modules.append(this);
    code    = mem.New<IrCodeSection>(this);
    data    = mem.New<IrDataSection>(this);
    imports = mem.New<IrImportSection>(this);
    exports = mem.New<IrExportSection>(this);
    strings = mem.New<IrStringTable>(this);
}

void IrModule::dispose() {
    code    = ndispose(code);
    data    = ndispose(data);
    imports = ndispose(imports);
    exports = ndispose(exports);
    strings = ndispose(strings);
    entry   = nullptr;
    __super::dispose();
}

bool IrModule::isaDll() {
    return binaryKind == BinaryKind::Dll;
}

bool IrModule::isExecutable() {
    return binaryKind != BinaryKind::Dll;
}
//------------------------------------------------------------------------------------------------
IrFunction::IrFunction(Loc loc, IrCodeSection *parentSection, Identifier name, Type retype)
    : IrTypeSymbol(loc, Kind::Function, name), parentSection(parentSection), retype(retype) {
    parentSection->functions.append(this);
    stack = comp.ir->mem.New<IrStackFrame>(this);
}

void IrFunction::dispose() {
    stack = ndispose(stack);
    ldispose(body);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
void IrBlock::dispose() {
    ldispose(body);
    __super::dispose();
}
//------------------------------------------------------------------------------------------------
IrUse::IrUse(IrOperation *user, IrNode *value, IrUseKind useKind)
    : user(user), value(value), useKind(useKind) {
    if (value) {
        value->uses.append(this);
    }
}
//------------------------------------------------------------------------------------------------
bool ModuleProvider::next(List<IrModule*> &batch) {
    AcquireSRWLockExclusive(&srw);
    auto end = comp.ir->modules.length;
    for (; pos < end; ++pos) {
        if (batch.length >= perBatch) {
            break;
        }
        auto mod = comp.ir->modules.items[pos];
        batch.append(mod);
    }
    ReleaseSRWLockExclusive(&srw);
    return batch.isNotEmpty();
}
//------------------------------------------------------------------------------------------------
#define ZM(zName, zSize) IrType IrTree::ty##zName{};
DeclareBuiltinTypeKeywords(ZM)
#undef ZM
} // namespace exy