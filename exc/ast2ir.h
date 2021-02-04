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

struct Builtin : SymbolTranslator {
    Builtin(SymTab symtab, Symbol &symbol) : SymbolTranslator(symtab, symbol) {}

    void translate() override {}
};

struct Module : SymbolTranslator {
    List<IrBlock*> freeBlocks{};

    Module(SymTab symtab, Symbol &symbol) : SymbolTranslator(symtab, symbol) {}
    void dispose() override;
    void translate() override;
};

struct Function : SymbolTranslator {
    Module          &parent;
    IrFunction      *fn;
    Queue<IrBlock>  &body;
    AstScope        *scope;
    IrBlock         *exit;

    Function(Module *parent, IrFunction *fn, AstScope *scope)
        : SymbolTranslator((*parent).symtab, (*parent).symbol), parent(*parent), fn(fn),
        body(fn->body), scope(scope) {
    }
    void dispose() override;
    void translate();
};

struct Scope {
    Scope    *parent{};
    Function &fn;
    Make      mk;
    IrBlock  *block{};

    Scope(Function *fn) : fn(*fn), mk(this) {}
    Scope(Scope *parent) : parent(parent), fn(parent->fn), mk(this), block(parent->block) {}
    void dispose();

    IrNode* visit(AstNode*);
private:
    IrNode* visitScope(AstScope*);
    IrNode* visitStatements(List<AstNode*>&);
    IrNode* visitDefinition(AstDefinition*);
    IrNode* visitCast(AstCast*);
    IrNode* visitConstant(AstConstant*);
    IrNode* visitName(AstName*);

    bool isRoot();
};
} // namespace ast2ir_pass
} // namespace exy

#endif // AST2IR_H