//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-02
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ir2pe.h"

#define err(token, msg, ...) print_error("Ir2Pe", token, msg, __VA_ARGS__)

namespace exy {
namespace codegen_pass {
void Module::visit() {
    traceln("generating %cl#<cyan> %s#<green> @thread(%u#<green>)",
            S("module"), mod->name, GetCurrentThreadId());
    CodeSection code{ this };
    code.visit();
    code.dispose();
}
//------------------------------------------------------------------------------------------------
void CodeSection::visit() {
    for (auto i = 0; i < section->functions.length; ++i) {
        visitFunction(section->functions.items[i], i);
    }
}

void CodeSection::visitFunction(IrFunction *node, int idx) {
    node->idx = idx;
    node->reg = Register::mkOff32(emit.pe.length);
    auto i = 0;
    for (auto block = node->body.first(); block; block = block->qnext) {
        visitBlock(block, i++);
    }
}

void CodeSection::visitBlock(IrBlock *node, int idx) {
    node->idx = idx;
    node->reg = Register::mkOff32(emit.pe.length);
    auto i = 0;
    for (auto operation = node->body.first(); operation; operation = operation->qnext) {
        visitOperation(operation, i++);
    }
}

void CodeSection::visitOperation(IrOperation *node, int idx) {
    node->idx = idx;
    switch (node->kind) {
    case IrKind::Exit: {
        visitExit((IrExit*)node);
    } break;
    default: {
        err(node, "emit %ir not implemented yet", node->kind);
    } break;
    }
}

void CodeSection::visitExit(IrExit*) {
    emit.ret();
}
} // namespace codegen_pass
} // namespace exy