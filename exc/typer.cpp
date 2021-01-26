//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-15
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#include "tp_literal.h"
#include "tp_import.h"
#include "tp_variable.h"

#define err(loc, msg, ...) print_error("Typer", loc, msg, __VA_ARGS__)

namespace exy {
namespace stx2ast_pass {
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
Typer::Typer() : tree(*comp.ast), mem(comp.ast->mem), mk(this), find(this) {
    comp.typer = this;
}

void Typer::dispose() {
    Assert(scopeStack.list.isEmpty());
    scopeStack.dispose();
    ldispose(_thrown);
    ldispose(_names);
    ldispose(_tpnames);
    ldispose(_constants);
    comp.typer = nullptr;
}

void Typer::run() {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, thread: %u#<green> }",
            S("typer"), S("building the AST"), GetCurrentThreadId());
    visitMain(tree.global);
    traceln("%cl#<cyan|blue> { errors: %i#<red>, modulesVisited: %i#<green> }",
            S("typer"), comp.errors, stats.modulesVisited);
}

SourceLocation Typer::mkpos(SyntaxNode *node) {
    return { node->pos.loc.file, node->pos.loc.range.start, node->lastpos().loc.range.end };
}

AstScope* Typer::currentScope() {
    Assert(scopeStack.list.length);
    return scopeStack.list.last();
}

AstModule* Typer::currentModule() {
    auto scope = currentScope();
    while (scope) {
        if (auto found = isa.Module(scope->owner)) {
            return found;
        }
    }
    Unreachable();
}

AstModule* Typer::moduleOf(AstSymbol *symbol) {
    if (!symbol) {
        return nullptr;
    } 
    auto scope = symbol->ownScope ? symbol->ownScope : symbol->parentScope;
    while (scope) {
        if (auto mod = isa.Module(scope->owner)) {
            return mod;
        }
        scope = scope->parent;
    }
    return nullptr;
}

bool Typer::visitMain(AstModule *sym) {
    auto errors = comp.errors;
    if (sym->name == ids.main) {
        visitModule(sym);
    } 
    auto &symbols = sym->ownScope->symbols;
    for (auto i = 0; i < symbols.length; ++i) {
        if (auto child = isa.Module(symbols.items[i].value)) {
            visitMain(child);
        }
    }
    return errors == comp.errors;
}

bool Typer::visitModule(AstModule *sym) {
    auto errors = comp.errors;
    auto  scope = sym->ownScope;
    if (scope->status.isIdle()) {
        if (scopeStack.push(scope)) {
            traceln("enter %cl#<cyan> %s#<green>", S("module"), sym->dotName);
            scope->status.begin();
            for (auto i = 0; i < sym->syntax.length; ++i) {
                auto file = sym->syntax.items[i];
                visitStatements(file->nodes);
            }
            scope->status.end();
            scopeStack.pop(scope);
            //traceln("exit %cl#<cyan> %s#<green>", S("module"), sym->dotName);
            ++stats.modulesVisited;
        }
    }
    return errors == comp.errors;
}

bool Typer::visitStatements(List<SyntaxNode*> &statements) {
    auto errors = comp.errors;
    for (auto i = 0; i < statements.length; ++i) {
        auto syntax = statements.items[i];
        visitStatement(syntax);
    }
    return errors == comp.errors;
}

bool Typer::visitStatement(SyntaxNode *syntax) {
    if (!syntax) {
        return true;
    }
    auto errors = comp.errors;
    switch (syntax->kind) {
        case SyntaxKind::Empty: {
            if (auto modifiers = syntax->modifiers) {
                err(modifiers, "stray modifiers");
            }
        } break;

        case SyntaxKind::Import:
        case SyntaxKind::Export: {
            Importer(this).visit((SyntaxImportOrExport*)syntax);
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
        default: if (auto node = visitExpression(syntax)) {
            if (isa.Name(node) || isa.TypeName(node) || isa.Constant(node)) {
                throwAway(node);
            } else {
                currentScope()->nodes.append(node);
            }
        } break;
    }
    return errors == comp.errors;
}

AstNode* Typer::visitExpression(SyntaxNode *syntax) {
    if (!syntax) {
        return nullptr;
    } switch (syntax->kind) {
        case SyntaxKind::Import:
        case SyntaxKind::Export: {
            err(syntax, "unexpected import/export");
        } break;

        case SyntaxKind::NameValue: if (syntax->modifiers) {
            return Variable(this).visit((SyntaxNameValue*)syntax);
        } else {
            Assert(0);
        } break;

        case SyntaxKind::UnarySuffix:
            return visitUnarySuffix((SyntaxUnarySuffix*)syntax);

        case SyntaxKind::Literal:
            return Literal(this).visit((SyntaxLiteral*)syntax);

        case SyntaxKind::Identifier:
            return find.name((SyntaxIdentifier*)syntax);

        default: {
            err(syntax, "%syntax not implemented", syntax->kind);
        } break;
    }
    return nullptr;
}

AstNode* Typer::visitUnarySuffix(SyntaxUnarySuffix *syntax) {
    auto value = visitExpression(syntax->value);
    if (!value) {
        return nullptr;
    } switch (syntax->op.kind) {
        case Tok::Pointer: if (auto tpname = isa.TypeName(value)) {
            throwAway(tpname);
            return mk.tpname(mkpos(syntax), tpname->type.pointer());
        } else {
            err(value->loc, "expected a typename before the %t", &syntax->op);
        } break;
        case Tok::Reference: if (auto tpname = isa.TypeName(value)) {
            throwAway(tpname);
            return mk.tpname(mkpos(syntax), tpname->type.reference());
        } else {
            err(value->loc, "expected a typename before the %t", &syntax->op);
        } break;
        default: {
            err(syntax, " unary-suffix operation %t not implemented", &syntax->op);
        } break;
    }
    return throwAway(value);
}
} // namespace stx2ast_pass
} // namespace exy