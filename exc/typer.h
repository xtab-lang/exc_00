//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-15
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TYPER_H_
#define TYPER_H_

#include "ast.h"
#include "syntax.h"

namespace exy {
namespace typ_pass {
//--Begin forward declarations
struct Typer;
//----End forward declarations
} // namespace typ_pass
} // namespace exy

#include "tp_find.h"
#include "tp_cast.h"
#include "tp_isa.h"
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
    AstTree   &tree;
    Mem       &mem;
    Make      mk;
    Importer  imp;
    Variable  var;
    Isa       isa;
    Find      find;
    Modifiers mods{};
    ScopeStack scopeStack{};
    List<AstNode*>     _thrown{};
    List<AstName*>     _names{};
    List<AstTypeName*> _tpnames{};

    Typer() : tree(*comp.ast), mem(comp.ast->mem), mk(this), imp(this), var(this), isa(this), find(this) {}
    void dispose();
    void run();

    SourceLocation mkpos(SyntaxNode*);
    AstScope* currentScope();
    AstModule* currentModule();

    bool visitMain(AstModule*);
    bool visitModule(AstModule*);

    bool visitStatements(List<SyntaxNode*>&);
    bool visitStatement(SyntaxNode*);
    AstNode* visitExpression(SyntaxNode*);
    AstNode* visitUnarySuffix(SyntaxUnarySuffix*);

    template<typename T>
    T* throwAway(T *node) {
        if (node) {
            if (auto name = isa.Name(node)) {
                _names.append(name);
            } else if (auto tpname = isa.TypeName(node)) {
                _tpnames.append(tpname);
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