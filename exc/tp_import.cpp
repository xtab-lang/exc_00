//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-16
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#include "tp_import.h"

#define err(loc, msg, ...) print_error("Import", loc, msg, __VA_ARGS__)

namespace exy {
namespace stx2ast_pass {
void Importer::visit(SyntaxImportOrExport *decl) {
	Modifiers::validateImportOrExportModifiers(decl->modifiers);

	Identifier name{};
	if (auto as = decl->as) {
		Assert(as->kind == SyntaxKind::Identifier);
		name = ((SyntaxIdentifier*)as)->value;
	} 
	AstSymbol *importedSymbol{};
	if (auto from = decl->from) {
		if (auto sourceModule = findSourceModule(from)) {
			importedSymbol = findSymbol(decl->name, sourceModule->ownScope);
		}
	} else {
		importedSymbol = findSymbol(decl->name);
	} if (importedSymbol) {
		if (decl->kind == SyntaxKind::Import) {
			createImport(decl, importedSymbol, name ? name : importedSymbol->name);
		} else if (decl->kind == SyntaxKind::Export) {
			createExport(decl, importedSymbol, name ? name : importedSymbol->name);
		}
	}
}

AstModule* Importer::findSourceModule(SyntaxNode *node) {
	if (auto found = findSymbol(node)) {
		if (found->kind == AstKind::Module) {
			return (AstModule*)found;
		}
		err(node, "not a module");
	}
	return nullptr;
}

AstSymbol* Importer::findSymbol(SyntaxNode *node, AstScope *scope) {
	if (node->kind == SyntaxKind::Identifier) {
		auto name = ((SyntaxIdentifier*)node)->value;
		if (auto found = scope->findThroughDot(name)) {
			return checkForSelfImportOrExport(node->pos, found);
		}
		err(node, "identifier %s#<red> not found in %s#<green>", name, scope->owner->name);
		return nullptr;
	} if (node->kind == SyntaxKind::DotExpression) {
		auto dotExpression = (SyntaxDotExpression*)node;
		if (auto lhs = dotExpression->lhs) {
			if (auto found = findSymbol(lhs)) {
				if (scope = found->ownScope) {
					return findSymbol(dotExpression->rhs, scope);
				}
				err(lhs, "name does not evaluate to a module in %s#<green>", scope->owner->name);
			}
		}
		err(dotExpression, "invalid lhs");
		return nullptr;
	}
	Unreachable();
}

AstSymbol* Importer::findSymbol(SyntaxNode *node) {
	if (node->kind == SyntaxKind::Identifier) {
		return findSymbol(node->pos, ((SyntaxIdentifier*)node)->value);
	} if (node->kind == SyntaxKind::DotExpression) {
		auto dotExpression = (SyntaxDotExpression*)node;
		if (auto lhs = dotExpression->lhs) {
			if (auto found = findSymbol(lhs)) {
				if (auto scope = found->ownScope) {
					return findSymbol(dotExpression->rhs, scope);
				}
				err(lhs, "name does not evaluate to a module");
			}
		} else {
			return findSymbol(dotExpression->rhs, tp.currentScope());
		}
		return nullptr;
	}
	Unreachable();
}

AstSymbol* Importer::findSymbol(Pos pos, Identifier name) {
	auto scope = tp.currentScope();
	while (scope) {
		if (auto found = scope->find(name)) {
			return checkForSelfImportOrExport(pos, found);
		}
		scope = scope->parent;
	}
	err(pos, "identifier %s#<red> not found", name);
	return nullptr;
}

void Importer::createImport(SyntaxImportOrExport *decl, AstSymbol *sym, Identifier name) {
	auto scope = tp.currentScope();
	if (auto found = scope->find(name)) {
		err(decl, "name %s#<red> already declared in the current scope", name);
	} else {
		tp.mk.importOf(tp.mkpos(decl), name, sym);
	}
}

void Importer::createExport(SyntaxImportOrExport *decl, AstSymbol *sym, Identifier name) {
	auto scope = tp.currentScope();
	if (auto found = scope->find(name)) {
		err(decl, "name %s#<red> already declared in the current scope", name);
	} else {
		tp.mk.exportOf(tp.mkpos(decl), name, sym);
	}
}

AstSymbol* Importer::checkForSelfImportOrExport(Pos pos, AstSymbol *symbol) {
	auto importedModule = tp.isa.Module(symbol);
	if (!importedModule) {
		if (auto alias = tp.isa.TypeAlias(symbol)) {
			return checkForSelfImportOrExport(pos, alias->type.isaSymbol());
		}
	} if (importedModule) {
		auto parent = tp.currentModule();
		while (parent) {
			if (importedModule == parent) {
				err(pos, "self-import/export");
				return nullptr;
			} if (auto scope = parent->ownScope->parent) {
				parent = (AstModule*)scope->owner;
				Assert(!parent || tp.isa.Module(parent));
			} else {
				break; // Top-level module.
			}
		}
	}
	return symbol;
}
} // namespace stx2ast_pass
} // namespace exy