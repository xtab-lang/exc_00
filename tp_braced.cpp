#include "pch.h"
#include "typer.h"

#define initializer_error(pos, msg, ...) diagnostic("Initializer", pos, msg, __VA_ARGS__)

namespace exy {
tp_braced::tp_braced(InitializerSyntax *syntax)
	: tp(*typer), pos(syntax), nameSyntax(syntax->name), argumentsSyntax(syntax->arguments) {}

void tp_braced::dispose() {
}

TpNode* tp_braced::bind(TpNode *base) {
	if (nameSyntax == nullptr) {
		Assert(0);
	}
	TpNode *result = nullptr;
	TpNode   *name = nullptr;
	if (base == nullptr) {
		name = tp.bindExpression(nameSyntax);
	} else {
		name = tp.bindDotExpression(base, nameSyntax);
	}
	if (name == nullptr) {
		return nullptr;
	}
	if (auto tpname = tp.isa.TypeName(name)) {
		result = bindTypeNameWithBraced(tpname);
	} else {
		initializer_error(nameSyntax, "expected a typename, not a value of %tptype", &name->type);
	}
	tp.throwAway(name);
	return result;
}

TpNode* tp_braced::bindTypeNameWithBraced(TpTypeName *name) {
	TpNode *result = nullptr;
	if (auto value = argumentsSyntax->value) {
		Assert(0);
	} else if (name->type.isaBuiltinOrIndirect() || name->type.isanEnum()) {
		result = tp.mk.ZeroOf(pos, name->type);
	} else if (name->type.isaStruct()) {
		result = tp.mk.StructInitializerFromSite(name->type);
	} else {
		impl_error(nameSyntax, "%tptype{}", &name->type);
	}
	return result;
}
} // namespace exy