#include "pch.h"
#include "typer.h"

#define index_error(pos, msg, ...) diagnostic("Index", pos, msg, __VA_ARGS__)

namespace exy {
tp_bracketed::tp_bracketed(IndexSyntax *syntax) 
	: tp(*typer), pos(syntax), nameSyntax(syntax->name), argumentsSyntax(syntax->arguments) {}

void tp_bracketed::dispose() {

}

TpNode* tp_bracketed::bind(TpNode *base) {
	if (nameSyntax == nullptr) {
		Assert(0);
	}
	tp_site site{ pos };
	TpNode *result = nullptr;

	if (base == nullptr) {
		result = tp.bindExpression(nameSyntax);
	} else {
		result = tp.bindDotExpression(base, nameSyntax);
	}

	if (result != nullptr) {
		if (auto tpname = tp.isa.TypeName(result)) {
			// T[ args ]
			result = bindIndexedTypename(tpname);
		} else {
			// t[ args ]
			result = bindIndexer(result);
		}
	}

	site.dispose();
	return result;
}

TpNode* tp_bracketed::bindIndexedTypename(TpTypeName *tpname) {
	auto &site = *tp.current->site;
	if (site.arguments.set(/* receiver = */ nullptr, argumentsSyntax, /* with = */ nullptr)) {
		if (site.arguments.list.isEmpty()) {
			// T[] translates to List<T>
			return bindList(tpname);
		}
		if (site.arguments.list.length == 1) {
			// T[ i: Integral ] translates to the fixed array [T × i]
			Assert(0);
		} else {
			index_error(argumentsSyntax, "expected 0 or 1 index arguments, not %i#<red>",
						site.arguments.list.length);
		}
	}
	return nullptr;
}

TpNode* tp_bracketed::bindList(TpTypeName *tpname) {
	// T[] translates to std.collections.List<T>
	TpNode *result = nullptr;
	auto	   mod = (TpModule*)tp.mod_collections->node;
	tp_lookup lookup{};
	if (auto found = lookup.find(pos, mod->scope, ids.kw_List)) {
		if (auto templateSymbol = found->type.isaStructTemplate()) {
			auto &site = *tp.current->site;
			site.arguments.append(pos, tpname);
			if (auto instanceSymbol = selectList(templateSymbol)) {
				result = tp.mk.Name(pos, instanceSymbol);
			}
		} else {
			index_error(pos, "not a std.collections.List<T> template: %tptype", &found->type);
			tp.throwAway(found);
		}
	}
	lookup.dispose();
	return result;
}

TpSymbol* tp_bracketed::selectList(TpSymbol *templateSymbol) {
	const auto errors = compiler.errors;
	auto        &site = *tp.current->site;
	auto templateNode = (TpTemplate*)templateSymbol->node;
	auto   syntaxNode = (StructureSyntax*)templateNode->syntax;
	if (!setSiteArgumentsAndParameters(templateSymbol, syntaxNode)) {
		return nullptr;
	}
	auto instanceSymbol = selectListInstance(templateSymbol);
	if (instanceSymbol == nullptr) {
		index_error(pos, "cannot instantiate %tptype with %tptype", &templateNode->type,
					&site.arguments.list.first().value->type);
		return nullptr;
	}
	return instanceSymbol;
}

bool tp_bracketed::setSiteArgumentsAndParameters(TpSymbol *templateSymbol, StructureSyntax *syntaxNode) {
	const auto errors = compiler.errors;
	auto        &site = *tp.current->site;
	if (site.parameters.set(templateSymbol->scope, syntaxNode->parameters)) {
		site.parameters.set(site.arguments);
	}
	return errors == compiler.errors;
}

TpSymbol* tp_bracketed::selectListInstance(TpSymbol * templateSymbol) {
	const auto errors = compiler.errors;
	auto        &site = *tp.current->site;
	auto instanceSymbol = site.selectInstance(templateSymbol);
	if (instanceSymbol == nullptr) {
		instanceSymbol = site.bindTemplate(templateSymbol);
		if (instanceSymbol != nullptr && errors == compiler.errors) {
			return instanceSymbol;
		}
	}
	return nullptr;
}

TpNode* tp_bracketed::bindFixedArray(TpTypeName *, TpConstExpr *) {
	Assert(0);
	return nullptr;
}

TpNode* tp_bracketed::bindIndexer(TpNode *name) {
	auto     &site = *tp.current->site;
	TpNode *result = nullptr;
	if (name->type.isNotAPointer()) {
		result = site.tryCallIndexer(name);
		if (result != nullptr) {
			return result;
		}
	}
	SyntaxNode *argSyntax = nullptr;
	TpNode      *argument = nullptr;
	if (site.arguments.set(/* receiver = */ nullptr, argumentsSyntax, /* with = */ nullptr)) {
		if (site.arguments.list.length == 1) {
			auto &arg = site.arguments.list.items[0];
			argument = arg.value;
			arg.value = nullptr;
			argSyntax = arg.syntax;
		} else {
			index_error(argumentsSyntax, "arguments for indexing: expected 1, got %i#<red>",
						site.arguments.list.length);
		}
	}
	if (argument == nullptr) {
		return tp.throwAway(name);
	}
	argument = tp.mk.DereferenceIfReference(argSyntax, argument);
	name = tp.mk.DereferenceIfReference(nameSyntax, name);
	// T*[ index: U/Int64]
	return tp.mk.IndexName(pos, name, argument);
}
} // namespace exy