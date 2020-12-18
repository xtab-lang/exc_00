//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-16
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(loc, msg, ...) print_error("Import", loc, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
void Importer::visit(Decl decl) {
	Modifiers::validateImportOrExportModifiers(decl->modifiers);

	Identifier name{};
	if (auto as = decl->as) {
		Assert(as->kind == SyntaxKind::Identifier);
		name = ((SyntaxIdentifier*)as)->value;
	} 
	AstSymbol *importedSymbol{};
	if (auto from = decl->from) {
		if (auto sourceModule = findSourceModule(from)) {
			importedSymbol = findSymbol(decl->name, sourceModule->scope);
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
			return found;
		}
		err(node, "identifier %s#<red> not found in %s#<green>", name, scope->name());
		return nullptr;
	} if (node->kind == SyntaxKind::DotExpression) {
		auto dotExpression = (SyntaxDotExpression*)node;
		if (auto lhs = dotExpression->lhs) {
			if (auto found = findSymbol(lhs)) {
				if (scope = found->scope) {
					return findSymbol(dotExpression->rhs, scope);
				}
				err(lhs, "name does not evaluate to a module in %s#<green>", scope->name());
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
				if (auto scope = found->scope) {
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
			return found;
		}
		scope = scope->parent;
	} if (auto found = comp.ast->find(name)) {
		return found;
	}
	err(pos, "identifier %s#<red> not found", name);
	return nullptr;
}

void Importer::createImport(Decl decl, AstSymbol *sym, Identifier name) {
	auto scope = tp.currentScope();
	if (auto found = scope->find(name)) {
		err(decl, "name %s#<red> already declared in the current scope", name);
	} else {
		comp.ast->mem.New<AstImport>(tp.mkpos(decl), scope, name, sym);
	}
}

void Importer::createExport(Decl decl, AstSymbol *sym, Identifier name) {
	auto scope = tp.currentScope();
	if (auto found = scope->find(name)) {
		err(decl, "name %s#<red> already declared in the current scope", name);
	} else {
		comp.ast->mem.New<AstExport>(tp.mkpos(decl), scope, name, sym);
	}
}
} // namespace typ_pass
} // namespace exy