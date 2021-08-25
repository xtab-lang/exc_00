#include "pch.h"
#include "typer.h"

#define import_error(pos, msg, ...) diagnostic("Import", pos, msg, __VA_ARGS__)

namespace exy {
struct ImportResolver {
	Typer  &tp;
	tp_isa &isa;

	ImportResolver() : tp(*typer), isa(typer->isa) {}

	TpSymbol* resolve(TpScope *scope, SyntaxNode *syntax) {
		if (auto dot = isa.DotSyntax(syntax)) {
			if (auto found = resolve(scope, dot->lhs)) {
				if (isa.ModuleSymbol(found)) {
					auto tpModule = (TpModule*)found->node;
					return resolve(tpModule->scope, dot->rhs);
				}
				import_error(syntax, "expected source of import to be a module not %tptype", 
							 &found->node->type);
			}
			return nullptr;
		} 
		if (auto id = isa.IdentifierSyntax(syntax)) {
			if (id->value == ids.kw_star) {
				impl_error(syntax, "import/export of '*' not yet implemented");
				return nullptr;
			} 
			if (scope == tp.current->scope) {
				auto p = scope;
				while (p) {
					if (auto found = p->contains(id->value)) {
						if (isa.ModuleSymbol(found)) {
							tp.bindModule(found);
						}
						return found;
					}
					p = p->parent;
				}
			} else if (auto found = scope->contains(id->value)) {
				if (isa.ModuleSymbol(found)) {
					tp.bindModule(found);
				}
				return found;
			}
			import_error(id, "identifier %s#<red> not found in %tptype",
						 id->value, &scope->owner->node->type);
			return nullptr;
		}
		syntax_error(syntax, "expected identifier or dot syntax, not %sk", syntax->kind);
		return nullptr;
	}
};

void Typer::bindImportStatement(ImportSyntax *syntax) {
	ImportResolver resolver{};
	TpScope *scope = nullptr;
	auto aliasKind = syntax->pos.keyword == Keyword::Import ? TpAliasKind::Import : TpAliasKind::Export;
	// (1) Resolve source of import.
	if (syntax->source != nullptr) {
		auto source = resolver.resolve(current->scope, syntax->source);
		if (isa.ModuleSymbol(source)) {
			auto tpModule = (TpModule*)source->node;
			scope = tpModule->scope;
		} else if (source != nullptr) {
			import_error(syntax->source, "expected source of import to be a module not %tptype", 
						 &source->node->type);
		}
	} else {
		scope = current->scope;
	}
	// (2) Resolve target from source.
	if (scope != nullptr) {
		if (auto   found = resolver.resolve(scope, syntax->name)) {
			auto      as = (IdentifierSyntax*)syntax->alias;
			auto    name = as == nullptr ? found->name : as->value;
			if (auto dup = current->scope->contains(name)) {
				dup_error(syntax->name, dup);
			} else if (isa.TypeAliasSymbol(found)) {
				auto alias = (TpTypeAlias*)found->node;
				if (alias->aliasKind == TpAliasKind::Export) {
					auto symbol = mk.TypeAlias(syntax->name, name, alias->type, aliasKind);
					applyBlockModifiers(symbol);
				} else {
					import_error(syntax->name, "cannot import an import");
				}
			} else if (isa.ConstAliasSymbol(found)) {
				auto alias = (TpConstAlias*)found->node;
				if (alias->aliasKind == TpAliasKind::Export) {
					auto symbol = mk.ConstAlias(syntax->name, name, alias->value, aliasKind);
					applyBlockModifiers(symbol);
				} else {
					import_error(syntax->name, "cannot import an import");
				}
			} else if (isa.ValueAliasSymbol(found)) {
				auto alias = (TpValueAlias*)found->node;
				if (alias->aliasKind == TpAliasKind::Export) {
					auto symbol = mk.ValueAlias(syntax->name, name, alias->value, aliasKind);
					applyBlockModifiers(symbol);
				} else {
					import_error(syntax->name, "cannot import an import");
				}
			} else if (isa.TypeSymbol(found)) {
				auto  value = (TpTypeNode*)found->node;
				auto symbol = mk.TypeAlias(syntax->name, name, value->type, aliasKind);
				applyBlockModifiers(symbol);
			} else if (isa.ValueSymbol(found)) {
				auto symbol = mk.ValueAlias(syntax->name, name, found, aliasKind);
				applyBlockModifiers(symbol);
			} else {
				import_error(syntax, "illegal import: %tptype", &found->node->type);
			}
		}
	}
}

void Typer::bindExportStatement(ImportSyntax *syntax) {
	bindImportStatement(syntax);
}
} // namespace exy