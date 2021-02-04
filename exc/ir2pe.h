//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-02
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef IR2PE_H
#define IR2PE_H

#include "ir.h"
#include "emitter.h"

namespace exy {
namespace codegen_pass {
struct Module {
    IrModule *mod;

    Module(IrModule *mod) : mod(mod) {}
    void dispose() {}
    void visit();
};

struct CodeSection {
    Module        &parent;
    IrCodeSection *section;
    Emitter        emit;

    CodeSection(Module *parent) : parent(*parent), section(parent->mod->code), 
        emit(parent->mod->code->peBuffer) {}
    void dispose() {}
    void visit();
private:
    void visitFunction(IrFunction*, int idx);
    void visitBlock(IrBlock*, int idx);
    void visitOperation(IrOperation*, int idx);

    void visitExit(IrExit*);
};

} // namespace codegen_pass
} // namespace exy

#endif // IR2PE_H