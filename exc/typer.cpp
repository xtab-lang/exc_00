//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-15
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(loc, msg, ...) print_error("Typer", loc, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
void ScopeStack::dispose() {
    list.dispose();
}
bool ScopeStack::push(AstScope *scope) {
    if (list.length >= comp.options.typer.maxScopeDepth) {
        err(scope->loc, "max. scope depth of %i#<red> reached", comp.options.typer.maxScopeDepth);
        return false;
    }
    list.append(scope);
    return true;
}
void ScopeStack::pop(AstScope *scope) {
    auto last = list.pop();
    Assert(last == scope);
}
//------------------------------------------------------------------------------------------------
void Typer::dispose() {
    scopeStack.dispose();
    ldispose(_thrown);
    ldispose(_names);
    ldispose(_tpnames);
}

void Typer::run() {
    for (auto i = 0; i < tree.symbols.length; ++i) {
        if (auto sym = tree.symbols.items[i].value->isaModule()) {
            visitMain(sym);
        }
    }
}

SourceLocation Typer::mkpos(SyntaxNode *node) {
    return { node->pos.loc.file, node->pos.loc.range.start, node->lastpos().loc.range.end };
}

AstScope* Typer::currentScope() {
    Assert(scopeStack.list.length);
    return scopeStack.list.last();
}

bool Typer::visitMain(AstModule *sym) {
    auto errors = comp.errors;
    if (sym->name == comp.str.main) {
        visitModule(sym);
    } for (auto i = 0; i < sym->scope->symbols.length; ++i) {
        if (auto child = sym->scope->symbols.items[i].value->isaModule()) {
            visitMain(child);
        }
    }
    return errors == comp.errors;
}

bool Typer::visitModule(AstModule *sym) {
    auto errors = comp.errors;
    if (sym->status.isIdle()) {
        if (scopeStack.push(sym->scope)) {
            traceln("enter %cl#<cyan> %s#<green>", S("module"), sym->dotName);
            sym->status.begin();
            for (auto i = 0; i < sym->syntax.length; ++i) {
                auto file = sym->syntax.items[i];
                visitStatements(file->nodes);
            }
            sym->status.end();
            scopeStack.pop(sym->scope);
            traceln("exit %cl#<cyan> %s#<green>", S("module"), sym->dotName);
        }
    }
    return errors == comp.errors;
}

bool Typer::visitStatements(Statements statements) {
    auto errors = comp.errors;
    for (auto i = 0; i < statements.length; ++i) {
        auto syntax = statements.items[i];
        visitStatement(syntax);
    }
    return errors == comp.errors;
}

bool Typer::visitStatement(Statement syntax) {
    if (!syntax) {
        return true;
    }
    auto errors = comp.errors;
    switch (syntax->kind) {
        case SyntaxKind::Import:
        case SyntaxKind::Export: {
            imp.visit((SyntaxImportOrExport*)syntax);
        } break;

        case SyntaxKind::CommaList: {
            auto list = (SyntaxCommaList*)syntax;
            for (auto i = 0; i < list->nodes.length; ++i) {
                auto child = list->nodes.items[i];
                if (child->modifiers) {
                    visitStatement(child);
                } else {
                    child->modifiers = list->modifiers;
                    visitStatement(child);
                    child->modifiers = nullptr;
                }
            }
        } break;
        default: if (auto expr = visitExpression(syntax)) {
            if (expr->isaName()) {
                throwAway(expr);
            }
        } break;
    }
    return errors == comp.errors;
}

AstNode* Typer::visitExpression(Expression syntax) {
    if (!syntax) {
        return nullptr;
    } switch (syntax->kind) {
        case SyntaxKind::Import:
        case SyntaxKind::Export: {
            err(syntax, "unexpected import/export");
        } break;
        case SyntaxKind::NameValue: if (syntax->modifiers) {
            return var.visit((SyntaxNameValue*)syntax);
        } {
            Assert(0);
        }
        case SyntaxKind::Literal:
            return lit.visit((SyntaxLiteral*)syntax);
        default: {
            err(syntax, "not implemented");
        } break;
    }
    return nullptr;
}
} // namespace typ_pass
} // namespace exy