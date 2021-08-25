#include "pch.h"
#include "typer.h"

namespace exy {
Typer::Typer() 
    : tree(*compiler.tpTree), mem(compiler.tpTree->mem), mk(this) {
	typer = this;
	_types.initialize();
}

void Typer::dispose() {
	Assert(current == nullptr);
	ldispose(_thrown);
	_types.dispose();
	typer = nullptr;
}

void Typer::run() {
	tp_current scope{ tree.scope };
	if (enter(scope)) {
		traceln("binding...");
		auto hasCreatedBuiltinAliases = false;
		for (auto i = 0; i < tree.modules.length; i++) {
			auto symbol = tree.modules.items[i];
			auto   node = (TpModule*)symbol->node;
			auto syntax = node->syntax;
			if (syntax->main != nullptr) {
				if (!hasCreatedBuiltinAliases) {
					makeBuiltinAliases(syntax->main);
					//--
					tp_lookup lookup{};
					auto ok = lookup.initialize(syntax->main);
					lookup.dispose();
					if (!ok) {
						break;
					}
					//--
					hasCreatedBuiltinAliases = true;
				}
				bindModule(symbol);
			}
		}
		traceln("...finished binding with %i#<red> error%c", 
				compiler.errors, compiler.errors == 1 ? "" : "s");
		leave(scope);
	}
}

void Typer::makeBuiltinAliases(SyntaxNode *syntax) {
	auto  ty = &tree.tyInt8;
	auto sym = ty->isDirect();
	mk.DefineTypeAlias(syntax, ids.get(S("Byte")), *ty);

	ty  = &tree.tyUInt8;
	sym = ty->isDirect();
	mk.DefineTypeAlias(syntax, ids.get(S("SByte")), *ty);

	ty  = &tree.tyInt16;
	sym = ty->isDirect();
	mk.DefineTypeAlias(syntax, ids.get(S("Short")), *ty);

	ty  = &tree.tyUInt16;
	sym = ty->isDirect();
	mk.DefineTypeAlias(syntax, ids.get(S("UShort")), *ty);

	ty  = &tree.tyInt32;
	sym = ty->isDirect();
	mk.DefineTypeAlias(syntax, ids.get(S("Int")), *ty);

	ty  = &tree.tyUInt32;
	sym = ty->isDirect();
	mk.DefineTypeAlias(syntax, ids.get(S("UInt")), *ty);

	ty = &tree.tyInt64;
	sym = ty->isDirect();
	mk.DefineTypeAlias(syntax, ids.get(S("Long")), *ty);

	ty = &tree.tyUInt64;
	sym = ty->isDirect();
	mk.DefineTypeAlias(syntax, ids.get(S("ULong")), *ty);
}

SourcePos Typer::mkPos(SyntaxNode *syntax) {
	auto &start = syntax->pos;
	auto   &end = syntax->lastPos();
	Assert(&start.pos.file == &end.pos.file);
	//if (start.pos.range == end.pos.range) {
		return SourcePos{ start.pos.file, start.pos.range.start, end.pos.range.end };
	//}
	//return SourcePos{ start.pos.file, start.pos.range.start, end.pos.range.start };
}

TpScope* Typer::getParentModuleScopeOf(TpScope *scope) {
	auto p = scope == nullptr ? current->scope : scope;
	while (p != nullptr) {
		if (isa.ModuleScope(p)) {
			return p;
		}
		p = p->parent;
	}
	UNREACHABLE();
}

TpSymbol* Typer::getTemplateSymbolOf(TpSymbol *instanceSymbol) {
	auto  name = instanceSymbol->name;
	auto scope = instanceSymbol->scope;
	if (auto found = scope->contains(name)) {
		switch (found->node->kind) {
			case TpKind::Template: {
				auto templateNode = (TpTemplate*)found->node;
				for (auto i = 0; i < templateNode->instances.length; i++) {
					auto instance = templateNode->instances.items[i];
					if (instance == instanceSymbol) {
						return found;
					}
				}
				Assert(0);
			} break;
			case TpKind::OverloadSet: {
				auto ov = (TpOverloadSet*)found->node;
				for (auto i = 0; i < ov->list.length; i++) {
					auto templateSymbol = ov->list.items[i];
					auto   templateNode = (TpTemplate*)templateSymbol->node;
					for (auto j = 0; j < templateNode->instances.length; j++) {
						auto instance = templateNode->instances.items[j];
						if (instance == instanceSymbol) {
							return templateSymbol;
						}
					}
				}
				Assert(0);
			} break;
		}
	}
	return nullptr;
}

TpTemplate* Typer::getTemplateNodeOf(TpSymbol *instanceSymbol) {
	if (auto templateSymbol = getTemplateSymbolOf(instanceSymbol)) {
		return (TpTemplate*)templateSymbol->node;
	}
	return nullptr;
}
} // namespace exy