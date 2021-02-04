//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-24
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ast2ir.h"

#define err(token, msg, ...) print_error("Ast2Ir", token, msg, __VA_ARGS__)

namespace exy {
namespace ast2ir_pass {
struct SymbolProvider : WorkProvider<Symbol> {
    SymTab symtab;
    int    pos{};

    SymbolProvider(SymTab symtab) : WorkProvider(), symtab(symtab) {}

    auto next(List<Symbol*> &batch) {
        AcquireSRWLockExclusive(&srw);
        auto end = symtab.list.length;
        for (; pos < end; ++pos) {
            if (batch.length >= perBatch) {
                break;
            }
            auto &sym = symtab.list.items[pos].value;
            batch.append(&sym);
        }
        ReleaseSRWLockExclusive(&srw);
        return batch.isNotEmpty();
    }
};

struct SymbolConsumer {
    SymTab  symtab;

    auto next(Symbol &symbol) {
        switch (symbol.ast->kind) {
        case AstKind::Builtin: {
            Builtin tp{ symtab, symbol };
            tp.translate();
            tp.dispose();
        } break;

        case AstKind::Module: {
            Module tp{ symtab, symbol };
            tp.translate();
            tp.dispose();
        } break;

        case AstKind::Global:
            break; // Do nothing.

        default: {
            err(symbol.ast, "ast → ir for %ast not implemented", symbol.ast->kind);
        } break;
        }
    }
};
//------------------------------------------------------------------------------------------------
void Module::dispose() {
    ldispose(freeBlocks);
}

void Module::translate() {
    traceln("ast → ir %type @thread(%i#<bold>)", &symbol.ast->type, GetCurrentThreadId());
    auto ast = (AstModule*)symbol.ast;
    auto  ir = (IrModule*)symbol.ir;
    Function tp{ this, ir->entry, ast->ownScope };
    tp.translate();
    tp.dispose();
}
//------------------------------------------------------------------------------------------------
void Function::dispose() {
}

void Function::translate() {
    Scope sc{ this };
    exit = sc.mk.block(scope->close, ids.exit);
    sc.block = sc.mk.block(scope->open, ids.entry);
    sc.visit(scope);
    sc.dispose();
}
//------------------------------------------------------------------------------------------------
void Scope::dispose() {
    if (isRoot()) {
        if (fn.body.isEmpty()) {
            mk.exit(fn.scope->close);
            fn.parent.freeBlocks.append(fn.exit);
        } else {
            Assert(0);
        }
    } else {
        parent->block = block;
    }
}

IrNode* Scope::visit(AstNode *ast) {
    if (!ast) {
        return nullptr;
    } switch (ast->kind) {
    case AstKind::Scope:
        return visitScope((AstScope*)ast);

    case AstKind::Definition:
        return visitDefinition((AstDefinition*)ast);

    case AstKind::Cast:
        return visitCast((AstCast*)ast);

    case AstKind::Constant:
        return visitConstant((AstConstant*)ast);

    case AstKind::Name:
        return visitName((AstName*)ast);

    default: {
        err(ast, "%ast not implemented", ast->kind);
    } break;
    }
    return nullptr;
}

IrNode* Scope::visitScope(AstScope *ast) {
    Scope sc{ this };
    auto last = sc.visitStatements(ast->nodes);
    sc.dispose();
    return last;
}

IrNode* Scope::visitStatements(List<AstNode*> &nodes) {
    IrNode *last{};
    for (auto i = 0; i < nodes.length; ++i) {
        last = visit(nodes.items[i]);
    }
    return last;
}

IrNode* Scope::visitDefinition(AstDefinition *ast) {
    // lhs ← rhs
    // lhs
    auto rhs = visit(ast->rhs);
    auto lhs = visit(ast->lhs);
    if (lhs && rhs) {
        mk.assign(ast->loc, lhs, rhs);
        return lhs;
    }
    return nullptr;
}

IrNode* Scope::visitCast(AstCast *ast) {
    // dst ← src as T
    // dst
    if (auto src = visit(ast->value)) {
        return mk.cast(ast->loc, mk.typeOf(ast->type), src);
    }
    return nullptr;
}

IrNode* Scope::visitConstant(AstConstant *ast) {
    // constant
    return mk.constant(ast->loc, mk.typeOf(ast->type), ast->u64);
}

IrNode* Scope::visitName(AstName *ast) {
    auto symbol = ast->symbol;
    switch (symbol->kind) {
    case AstKind::Global: {
        // module.data[global]
        auto  base = mk.moduleOf(symbol);
        auto index = mk.symbolOf(symbol);
        return mk.path(ast->loc, base, index);
    }

    default: {
        err(ast, "%ast not implemented", ast->kind);
    } break;
    }
    return nullptr;
}

bool Scope::isRoot() {
    return parent == nullptr;
}
//------------------------------------------------------------------------------------------------
static bool buildSymbolTable(SymTab symtab) {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, thread: %u#<green> }",
            S("ast2ir"), S("building the ast → ir symbol table"), GetCurrentThreadId());

    SymbolTableBuilder builder{ symtab };
    builder.visitTree();

    auto &ir = *comp.ir;
    traceln("%cl#<cyan|blue> { errors: %i#<red>, symbols: %i#<green>, modules: %i#<green> }",
            S("ast2ir"), comp.errors, symtab.list.length, ir.modules.length);

    return comp.errors == 0;
}

static bool visitSymbolTable(SymTab symtab) {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, threads: %u#<green> }",
            S("ast2ir"), S("ast → ir"), aio::ioThreads());

    SymbolProvider provider{ symtab };
    SymbolConsumer consumer{ symtab };
    aio::run(consumer, provider);

    traceln("%cl#<cyan|blue> { errors: %i#<red> }",
            S("ast2ir"), comp.errors);

    return comp.errors == 0;
}

void translateAstTree() {
    SymbolTable symtab{};

    if (buildSymbolTable(symtab)) {
        if (visitSymbolTable(symtab)) {

        }
    }

    symtab.dispose();
}
} // namespace irg_pass
} // namespace exy