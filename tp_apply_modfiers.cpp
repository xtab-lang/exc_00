#include "pch.h"
#include "typer.h"

namespace exy {
void Typer::applyBlockModifiers(TpSymbol *symbol) {
	for (auto i = 0; i < current->modifiers.length; i++) {
		applyModifiers(current->modifiers.items[i], symbol);
	}
}

void Typer::applyModifiers(SyntaxNode *modifiers, TpSymbol *symbol) {
	if (modifiers == nullptr) {
		return;
	}
	if (auto list = isa.ModifierListSyntax(modifiers)) {
		for (auto i = 0; i < list->nodes.length; i++) {
			applyModifier((ModifierSyntax*)list->nodes.items[i], symbol);
		}
		return;
	}
	if (auto modifier = isa.ModifierSyntax(modifiers)) {
		applyModifier(modifier, symbol);
		return;
	}
	impl_error(modifiers, "expected modifiers, not %sk", modifiers->kind);
}

void Typer::applyModifier(ModifierSyntax *modifier, TpSymbol *symbol) {
	auto node = symbol->node;
	switch (modifier->value) {
	#define ZM(zName, zText) case Keyword::zName: if (symbol->node->modifiers.is##zName) { \
			syntax_error(modifier, "modifier %kw already applied to %s#<green>: %tptype", modifier->value, symbol->name, &node->type); \
		} else { \
			symbol->node->modifiers.is##zName = true; \
		} return;
		DeclareModifiers(ZM)
		#undef ZM
	}
	UNREACHABLE();
}
} // namespace exy