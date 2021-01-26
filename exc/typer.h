//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-15
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef STX2AST_TYPER_H_
#define STX2AST_TYPER_H_

#include "ast.h"
#include "syntax.h"

namespace exy {
namespace stx2ast_pass {
//--Begin forward declarations
struct Typer;
//----End forward declarations
} // namespace stx2ast_pass
} // namespace exy

#include "tp_find.h"
#include "tp_isa.h"
#include "tp_mk.h"
#include "tp_modifiers.h"

namespace exy {
namespace stx2ast_pass {
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
    Find      find;
    Isa       isa{};
    Modifiers mods{};
    ScopeStack scopeStack{};
    List<AstNode*>     _thrown{};
    List<AstName*>     _names{};
    List<AstTypeName*> _tpnames{};
    List<AstConstant*> _constants{};
    struct {
        int modulesVisited{};
    } stats{};

    Typer();
    void dispose();
    void run();

    SourceLocation mkpos(SyntaxNode*);
    AstScope* currentScope();
    AstModule* currentModule();
    AstModule* moduleOf(AstSymbol*);

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
            } else if (auto constant = isa.Constant(node)) {
                _constants.append(constant);
            } else {
                _thrown.append(node);
            }
        }
        return (T*)nullptr;
    }
};
} // namespace stx2ast_pass
} // namespace exy

#endif // STX2AST_TYPER_H_