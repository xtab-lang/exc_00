//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-29
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef AST2IR_SYMTAB_H
#define AST2IR_SYMTAB_H

namespace exy {
namespace ast2ir_pass {
struct Symbol {
    IrModule  *parent;
    AstSymbol *ast;
    IrSymbol  *ir;

    Symbol(IrModule *parent, AstSymbol *ast, IrSymbol *ir) : parent(parent), ast(ast), ir(ir) {}
};
//------------------------------------------------------------------------------------------------
struct SymbolTable {
    Dict<Symbol, UINT64> list{};

    void dispose();

    IrBuiltin* append(IrModule *parent, AstBuiltin*);
    IrModule*  append(AstModule*);
    IrGlobal*  append(IrModule *parent, AstGlobal*, const IrType &type);

    IrSymbol* get(AstSymbol*);
    IrSymbol* find(AstSymbol*);
private:
    static Identifier fullNameOf(AstSymbol*);
};
using SymTab = SymbolTable&;
//------------------------------------------------------------------------------------------------
struct SymbolTableBuilder {
    SymTab symtab;
    Mem   &mem;

    SymbolTableBuilder(SymTab symtab) : symtab(symtab), mem(comp.ir->mem) {}
    void visitTree();
private:
    void visitSymbols(Dict<AstSymbol*>&);
    void visitSymbols(List<AstSymbol*>&);
    void visitScope(AstScope*);
    IrSymbol*  visitSymbol(AstSymbol*);
    IrBuiltin* visitBuiltin(AstBuiltin*);
    IrModule*  visitModule(AstModule*);
    IrGlobal*  visitGlobal(AstGlobal*);

    IrType     typeOf(const AstType&);
    IrModule*  moduleOf(AstSymbol*);
};
} // namespace ast2ir_pass
} // namespace exy

#endif // AST2IR_SYMTAB_H