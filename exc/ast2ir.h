//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-24
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef AST2IR_H
#define AST2IR_H

#include "ast.h"
#include "ir.h"

#include "ast2ir_symtab.h"
#include "ast2ir_mk.h"

namespace exy {
namespace ast2ir_pass {
void translateAstTree();


struct SymbolTranslator {
    SymTab  symtab;
    Symbol &symbol;

    SymbolTranslator(SymTab symtab, Symbol &symbol) : symtab(symtab), symbol(symbol) {}

    virtual void dispose() {}
    virtual void translate() = 0;
};

struct BuiltinTranslator : SymbolTranslator {
    BuiltinTranslator(SymTab symtab, Symbol &symbol) : SymbolTranslator(symtab, symbol) {}

    void translate() override {}
};

struct ModuleTranslator : SymbolTranslator {
    List<IrBlock*> freeBlocks{};

    ModuleTranslator(SymTab symtab, Symbol &symbol) : SymbolTranslator(symtab, symbol) {}
    void dispose() override;
    void translate() override;
};

struct FunctionTranslator : SymbolTranslator {
    ModuleTranslator &parent;
    IrFunction       *fn;
    Queue<IrBlock>   &body;
    AstScope         *scope;
    IrBlock          *exit;

    FunctionTranslator(ModuleTranslator *parent, IrFunction *fn, AstScope *scope)
        : SymbolTranslator((*parent).symtab, (*parent).symbol), parent(*parent), fn(fn),
        body(fn->body), scope(scope) {
    }
    void translate();
};

struct Scope {
    FunctionTranslator &fn;
    Make                mk;
    IrBlock            *block{};

    Scope(FunctionTranslator *fn) : fn(*fn), mk(this) {}
    void dispose();

    IrNode* visit(AstNode*);
private:
    IrNode* visitScope(AstScope*);
    IrNode* visitDefinition(AstDefinition*);
    IrNode* visitCast(AstCast*);
    IrNode* visitConstant(AstConstant*);
    IrNode* visitName(AstName*);
};
} // namespace ast2ir_pass
} // namespace exy

#endif // AST2IR_H