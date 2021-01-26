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
            BuiltinTranslator tp{ symtab, symbol };
            tp.translate();
            tp.dispose();
        } break;

        case AstKind::Module: {
            ModuleTranslator tp{ symtab, symbol };
            tp.translate();
            tp.dispose();
        } break;

        case AstKind::Global:
            break; // Do nothing.

        default: {
            err(symbol.ast, "%ast not implemented", symbol.ast->kind);
        } break;
        }
    }
};
//------------------------------------------------------------------------------------------------
void ModuleTranslator::dispose() {
    ldispose(freeBlocks);
}

void ModuleTranslator::translate() {
    traceln("ast → ir %type @thread(%i#<bold>)", &symbol.ast->type, GetCurrentThreadId());
    auto ast = (AstModule*)symbol.ast;
    auto  ir = (IrModule*)symbol.ir;
    FunctionTranslator tp{ this, ir->entry, ast->ownScope };
    tp.translate();
    tp.dispose();
}
//------------------------------------------------------------------------------------------------
void FunctionTranslator::translate() {
    Scope sc{ this };
    exit = sc.mk.block(scope->close, ids.exit);
    sc.block = sc.mk.block(scope->open, ids.entry);
    sc.visit(scope);
    sc.dispose();
}
//------------------------------------------------------------------------------------------------
void Scope::dispose() {
    if (fn.body.isEmpty()) {
        fn.body.append(block);
        fn.parent.freeBlocks.append(fn.exit);
    }
    else {
        Assert(0);
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
    for (auto i = 0; i < ast->nodes.length; ++i) {
        visit(ast->nodes.items[i]);
    }
    return nullptr;
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
    // base[index]
    auto symbol = ast->symbol;
    switch (symbol->kind) {
    case AstKind::Global: {
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

	auto &ir = *comp.ir;
	traceln("%cl#<cyan|blue> { errors: %i#<red>, modules: %i#<green> }",
			S("ast2ir"), comp.errors, ir.modules.length);

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