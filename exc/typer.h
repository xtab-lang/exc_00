//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-15
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TYPER_H_
#define TYPER_H_

#include "ast.h"
#include "syntax.h"

#include "tp_mk.h"
#include "tp_import.h"
#include "tp_literal.h"
#include "tp_variable.h"
#include "tp_modifiers.h"

namespace exy {
namespace typ_pass {
//--Begin forward declarations
//----End forward declarations
struct ScopeStack {
    List<AstScope*> list;
    void dispose();
    bool push(AstScope*);
    void pop(AstScope*);
};

struct Typer {
    using Statement = SyntaxNode*;
    using Expression = SyntaxNode*;
    using Statements = List<Statement>&;

    AstTree   &tree;
    Mem       &mem;
    Make      mk;
    Importer  imp;
    Literal   lit;
    Variable  var;
    Modifiers mods{};
    ScopeStack scopeStack{};
    List<AstNode*>     _thrown{};
    List<AstName*>     _names{};
    List<AstTypeName*> _tpnames{};

    Typer() : tree(*comp.ast), mem(comp.ast->mem), mk(this), imp(this), lit(this), var(this) {}
    void dispose();
    void run();

    SourceLocation mkpos(SyntaxNode*);
    AstScope* currentScope();

    bool visitMain(AstModule*);
    bool visitModule(AstModule*);

    bool visitStatements(Statements);
    bool visitStatement(Statement);
    AstNode* visitExpression(Expression);

    template<typename T>
    T* throwAway(T *node) {
        if (node) {
            if (node->kind == AstKind::Name) {
                _names.append((AstName*)node);
            } else if (node->kind == AstKind::TypeName) {
                _tpnames.append((AstTypeName*)node);
            } else {
                _thrown.append(node);
            }
        }
        return (T*)nullptr;
    }
};
} // namespace typ_pass
} // namespace exy

#endif // TYPER_H_