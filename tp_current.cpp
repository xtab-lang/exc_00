#include "pch.h"
#include "typer.h"
#include "tp_current.h"

#define MAX_SCOPE_DEPTH 0x100

#define depth_err(pos, msg, ...) diagnostic("ScopeDepth", pos, msg, __VA_ARGS__)

namespace exy {
tp_current::tp_current(TpScope *scope)
	: tp(*typer), prev(typer->current), scope(scope), depth(prev == nullptr ? 0 : prev->depth + 1) {}

void tp_current::dispose() {
	Assert(modifiers.isEmpty());
	modifiers.dispose();
	Assert(site == nullptr);
}

void tp_current::pushBlockModifiers(SyntaxNode *syntax) {
	if (syntax != nullptr) {
		modifiers.append(syntax);
	}
}

void tp_current::popBlockModifiers(SyntaxNode *syntax) {
	if (syntax != nullptr) {
		auto x = modifiers.pop();
		Assert(x == syntax);
	}
}

ExternBlockSyntax* tp_current::pushExternBlock(ExternBlockSyntax *block) {
	auto p = ext;
	ext = block;
	pushBlockModifiers(ext->modifiers);
	return p;
}

void tp_current::popExternBlock(ExternBlockSyntax *p, ExternBlockSyntax *block) {
	Assert(ext == block);
	popBlockModifiers(ext->modifiers);
	ext = p;
}

void tp_current::pushStructModifiers(SyntaxNode *syntax) {
	if (syntax != nullptr) {
		if (auto modifier = tp.isa.ModifierSyntax(syntax)) {
			switch (modifier->value) {
				case Keyword::Static:
				case Keyword::Const:
				case Keyword::ReadOnly:
				case Keyword::Auto: {
					modifiers.append(modifier);
				} break;
			}
		} else if (auto list = tp.isa.ModifierListSyntax(syntax)) {
			for (auto i = 0; i < list->nodes.length; i++) {
				pushStructModifiers(list->nodes.items[i]);
			}
		} else {
			Assert(0);
		}
	}
}

void tp_current::popStructModifiers(SyntaxNode *syntax) {
	if (syntax != nullptr) {
		if (auto modifier = tp.isa.ModifierSyntax(syntax)) {
			switch (modifier->value) {
				case Keyword::Static:
				case Keyword::Const:
				case Keyword::ReadOnly:
				case Keyword::Auto: {
					auto x = modifiers.pop();
					Assert(x == modifier);
				} break;
			}
		} else if (auto list = tp.isa.ModifierListSyntax(syntax)) {
			for (auto i = list->nodes.length; --i >= 0; ) {
				popStructModifiers(list->nodes.items[i]);
			}
		} else {
			Assert(0);
		}
	}
}

void tp_current::pushFnModifiers(SyntaxNode *syntax) {
	if (syntax != nullptr) {
		if (auto modifier = tp.isa.ModifierSyntax(syntax)) {
			switch (modifier->value) {
				case Keyword::Static:
				case Keyword::Const:
				case Keyword::Async:
				case Keyword::Auto: {
					modifiers.append(modifier);
				} break;
				default: {
					syntax_error(modifier, "%kw not allowed on %kw", modifier->value, Keyword::Fn);
				} break;
			}
		} else if (auto list = tp.isa.ModifierListSyntax(syntax)) {
			for (auto i = 0; i < list->nodes.length; i++) {
				pushFnModifiers(list->nodes.items[i]);
			}
		} else {
			Assert(0);
		}
	}
}

void tp_current::popFnModifiers(SyntaxNode *syntax) {
	if (syntax != nullptr) {
		if (auto modifier = tp.isa.ModifierSyntax(syntax)) {
			switch (modifier->value) {
				case Keyword::Static:
				case Keyword::Const:
				case Keyword::Async:
				case Keyword::Auto: {
					auto x = modifiers.pop();
					Assert(x == modifier);
				} break;
			}
		} else if (auto list = tp.isa.ModifierListSyntax(syntax)) {
			for (auto i = list->nodes.length; --i >= 0; ) {
				popFnModifiers(list->nodes.items[i]);
			}
		} else {
			Assert(0);
		}
	}
}

TpLabel* tp_current::isaLoop() {
	auto p = scope;
	while (p != nullptr) {
		if (auto owner = p->owner) {
			if (auto label = tp.isa.Label(owner)) {
				if (label->block->kind == TpKind::Loop) {
					return label;
				}
				p = p->parent;
				continue;
			}
		} 
		break;
	}
	return nullptr;
}

bool tp_current::isaLocalsScope() {
	if (auto owner = scope->owner) {
		const auto k = owner->node->kind;
		return k == TpKind::Function || k == TpKind::Block;
	}
	return false;
}

bool tp_current::isStatic() {
	auto p = this;
	while (p != nullptr) {
		if (auto owner = p->scope->owner) {
			if (owner->node->modifiers.isStatic) {
				return true;
			}
		}
		for (auto i = 0; i < p->modifiers.length; i++) {
			if (tp.isa.kwStatic(p->modifiers.items[i])) {
				return true;
			}
		}
		if (tp.isa.Label(p->scope->owner)) {
			p = p->prev; // Continue up the scope hierarchy until the first non-block scope.
		} else {
			break; // Stop at the first non-block scope.
		}
	}
	return false;
}

TpSymbol* tp_current::lambdaFunction() {
	if (auto fnSymbol = function()) {
		return tp.isa.LambdaFunctionSymbol(fnSymbol);
	}
	return nullptr;
}

TpSymbol* tp_current::function() {
	auto p = scope;
	while (p != nullptr) {
		if (auto owner = p->owner) {
			if (tp.isa.Label(p->owner)) {
				p = p->parent; // Continue up the scope hierarchy until the first non-block scope.
			} else if (tp.isa.Function(owner)) {
				return owner;
			} else {
				break; // Stop at the first non-block scope.
			}
		} else {
			break;
		}
	}
	return nullptr;
}

//----------------------------------------------------------
bool Typer::enter(tp_current &next) {
	if (next.depth > MAX_SCOPE_DEPTH) {
		depth_err(next.scope->owner, "maximum scope depth of %i#<green> has been breached",
				  MAX_SCOPE_DEPTH);
		return false;
	}
	if (current != nullptr) {
		next.site = current->site;
	}
	current = &next;
	return true;
}

void Typer::leave(tp_current &next) {
	Assert(&next == current);
	if (next.prev == nullptr) {
		Assert(next.site == nullptr);
	} else {
		Assert(next.site == next.prev->site);
		next.site = nullptr; // Because, {tp.enter} set {next.site}.
	}
	next.dispose();
	current = next.prev;
}

} // namespace exy