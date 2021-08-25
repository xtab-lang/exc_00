#include "pch.h"
#include "typer.h"

namespace exy {
tp_module::tp_module(TpSymbol *moduleSymbol) 
    : tp(*typer), moduleSymbol(moduleSymbol) {}

void tp_module::dispose() {}

void tp_module::bind() {
	const auto errors = compiler.errors;
	if (moduleSymbol->bindStatus.begin()) {
		auto node = (TpModule*)moduleSymbol->node;
		traceln("%depth binding %tptype", &node->type);
		tp_current scope{ node->scope };
		if (tp.enter(scope)) {
			auto syntax = node->syntax;
			// (1) Collect type declarations in each file at file scope.
			for (auto i = 0; i < syntax->files.length; i++) {
				auto file = syntax->files.items[i];
				tp_site site{ file };
				tp.collectTemplates(file->nodes);
				site.dispose();
			}
			// (2) Bind each non-type declaration statement in the module's {init} and {main} files.
			if (errors == compiler.errors) {
				if (auto file = syntax->init) {
					tp_site site{ file };
					bindStatements(file->nodes);
					site.dispose();
				}
				if (auto file = syntax->main) {
					tp_site site{ file };
					bindStatements(file->nodes);
					site.dispose();
				}
				if (errors == compiler.errors) {
					findAndBindMain();
				}
			}
			tp.leave(scope);
		}
		moduleSymbol->bindStatus.finish();
		traceln("%depth bound   %tptype (with %i#<red> error%c)", &node->type,
				compiler.errors - errors, compiler.errors - errors == 1 ? "" : "s");
	}
}

void tp_module::bindStatements(SyntaxNodes statements) {
	for (auto i = 0; i < statements.length; i++) {
		bindStatement(statements.items[i]);
	}
}

void tp_module::bindStatement(SyntaxNode *syntax) {
	switch (syntax->kind) {
		case SyntaxKind::Empty:
			break; // Ok but do nothing.
		case SyntaxKind::Module:
			break; // Ok but do nothing.
		case SyntaxKind::Import: {
			tp.bindImportStatement((ImportSyntax*)syntax);
		} break;
		case SyntaxKind::Export: {
			tp.bindExportStatement((ImportSyntax*)syntax);
		} break;
		case SyntaxKind::Define: {
			tp.bindDefineStatement((DefineSyntax*)syntax);
		} break;
		case SyntaxKind::ExternBlock: {
			auto block = (ExternBlockSyntax*)syntax;
			auto  prev = tp.current->pushExternBlock(block);
			for (auto i = 0; i < block->nodes.length; i++) {
				bindStatement(block->nodes.items[i]);
			}
			tp.current->popExternBlock(prev, block);
		} break;
		case SyntaxKind::Structure:
		case SyntaxKind::Function: {
			// Do nothing. Already collected as templates.
		} break;
		case SyntaxKind::Block: {
			auto block = (BlockSyntax*)syntax;
			tp.current->pushBlockModifiers(block->modifiers);
			for (auto i = 0; i < block->nodes.length; i++) {
				bindStatement(block->nodes.items[i]);
			}
			tp.current->popBlockModifiers(block->modifiers);
		} break;
		case SyntaxKind::Variable: {
			tp.bindGlobal((VariableSyntax*)syntax);
		} break;
		case SyntaxKind::CommaSeparated: {
			auto list = (CommaSeparatedSyntax*)syntax;
			for (auto i = 0; i < list->nodes.length; i++) {
				bindStatement(list->nodes.items[i]);
			}
		} break;
		default:
			syntax_error(syntax, "%sk not expected in file scope", syntax->kind);
			break;
	}
}

void tp_module::findAndBindMain() {
	auto symbol = tp.current->scope->contains(ids.kw_main);
	if (symbol == nullptr) {
		return;
	}
	if (auto node = tp.isa.Template(symbol)) {
		if (auto syntax = tp.isa.FunctionSyntax(node->syntax)) {
			if (tp.isa.kwFn(syntax->pos.keyword) && node->arity.isZero()) {
				auto mod = (TpModule*)moduleSymbol->node;
				mod->main = symbol;
				return bindMain(symbol);
			}
		}
	}
	type_error(symbol, "%tptype is not a valid entry point: check that\
\r\n\t(1) it is declared as %kw without any modifiers\
\r\n\t(2) it has no parameters", &symbol->node->type, Keyword::Fn);
}

void tp_module::bindMain(TpSymbol *templateSymbol) {
	auto templateNode = (TpTemplate*)templateSymbol->node;
	auto   syntaxNode = (FunctionSyntax*)templateNode->syntax;
	tp_site site{ syntaxNode };
	if (auto instanceSymbol = site.bindTemplate(templateSymbol)) {
		auto   instanceNode = (TpFunction*)instanceSymbol->node;
		if (instanceNode->modifiers.isAsync) { // 'async' main
			if (site.createAsyncMain(syntaxNode, instanceSymbol)) {
				auto mod = (TpModule*)moduleSymbol->node;
				mod->main = instanceSymbol;
			}
		}
	}
	site.dispose();
}

//----------------------------------------------------------
void Typer::bindModule(TpSymbol *moduleSymbol) {
	tp_module mod{ moduleSymbol };
	mod.bind();
	mod.dispose();
}
}