#include "pch.h"
#include "typer.h"

namespace exy {
void Typer::bindDefineStatement(DefineSyntax *syntax) {
	auto    name = syntax->name->value;
	if (auto dup = current->scope->contains(name)) {
		dup_error(syntax->name, dup);
		return;
	}
	if (auto node = bindExpression(syntax->value)) {
		if (auto constant = isa.ConstExpr(node)) {
			if (auto symbol = mk.DefineConstAlias(syntax, name, constant)) {
				applyBlockModifiers(symbol);
				applyModifiers(syntax->modifiers, symbol);
			}
		} else if (auto tpname = isa.TypeName(node)) {
			if (auto symbol = mk.TypeAlias(syntax, name, tpname->type, TpAliasKind::Define)) {
				applyBlockModifiers(symbol);
				applyModifiers(syntax->modifiers, symbol);
			}
		} else {
			impl_error(syntax, "bindDefine(%tpk)", node->kind);
			throwAway(node);
		}
	}
}

void Typer::bindYieldStatement(UnaryPrefixSyntax *syntax) {
	if (auto value = bindExpression(syntax->expression)) {
		if (syntax->kwFrom != nullptr) { // 'yield from'
			if (value = mk.YieldFrom(syntax, value)) {
				current->scope->append(value);
			}
		} else if (value = mk.Yield_(syntax, value)) {
			current->scope->append(value);
		}
	}
}

void Typer::bindAssertStatement(FlowControlSyntax *syntax) {
	/*if (auto list = isa.CommaSeparatedSyntax(syntax->expression)) {
		// Syntax → 'assert' expression (',' expression)+
		const auto errors = compiler.errors;
		List<TpNode*> arguments{};
		for (auto i = 0; i < list->nodes.length; i++) {
			if (auto argument = bindCondition(list->nodes.items[i])) {
				arguments.append(argument);
			}
		}
		if (errors != compiler.errors) {
			throwAway(arguments);
			return;
		}
	} else if (auto condition = bindExpression(syntax->expression)) {
		condition = mk.Condition(syntax->expression, condition, Tok::Equal, mk.ZeroOf(syntax->expression, condition->type));
		if (condition != nullptr) {
			// 'if' condition == false {
			//		
			// }
		}
	}*/
}

void Typer::bindReturnStatement(FlowControlSyntax *syntax) {
	TpNode *ret = nullptr;
	if (auto expression = syntax->expression) {
		TpNode *value = nullptr;
		if (syntax->kwIf) {
			// Syntax → 'return' 'if' expression
			// Translation 🡓
			//		if expression {
			//			return
			//		}
			auto block = mk.IfBlock(syntax);
			tp_current cur{ block->scope };
			if (enter(cur)) {
				block->condition = bindCondition(expression);
				current->scope->append(mk.Return(syntax, value));
				leave(cur);
			}
			return;
		} 
		if (auto ifexpr = isa.IfExpressionSyntax(expression)) {
			if (ifexpr->ifalse == nullptr) {
				// Syntax → 'return' value 'if' condition
				// Translation 🡓
				//		'if' expression {
				//			return value
				//		}
				auto block = mk.IfBlock(syntax);
				tp_current cur{ block->scope };
				if (enter(cur)) {
					block->condition = bindCondition(expression);
					if (value = bindExpression(ifexpr->iftrue)) {
						current->scope->append(mk.Return(syntax, value));
					}
					leave(cur);
				}
				return;
			}
			// 'return' expression 'if' expression 'else' expression
			value = bindIfExpression(ifexpr);
		} else {
			// 'return' expression
			value = bindExpression(expression);
		}
		if (value != nullptr) {
			ret = mk.Return(syntax, value);
		}
	} else {
		// 'return'
		ret = mk.Return(syntax, nullptr);
	}
	if (ret != nullptr) {
		current->scope->append(ret);
	}
}

void Typer::bindBreakStatement(FlowControlSyntax *syntax) {
	if (auto condition = syntax->expression) {
		Assert(syntax->kwIf != nullptr);
		// Syntax → 'break' 'if' condition
		// Translation 🡓
		//		if expression {
		//			break
		//		}
		auto block = mk.IfBlock(syntax);
		tp_current cur{ block->scope };
		if (enter(cur)) {
			block->condition = bindCondition(condition);
			current->scope->append(mk.Break(syntax));
			leave(cur);
		}
	} else {
		current->scope->append(mk.Break(syntax));
	}
}

void Typer::bindContinueStatement(FlowControlSyntax *syntax) {
	if (auto condition = syntax->expression) {
		Assert(syntax->kwIf != nullptr);
		// Syntax → 'continue' 'if' condition
		// Translation 🡓
		//		if expression {
		//			continue
		//		}
		auto block = mk.IfBlock(syntax);
		tp_current cur{ block->scope };
		if (enter(cur)) {
			block->condition = bindCondition(condition);
			current->scope->append(mk.Continue(syntax));
			leave(cur);
		}
	} else {
		current->scope->append(mk.Continue(syntax));
	}
}

void Typer::bindIfStatement(IfSyntax *syntax) {
	tp_if_block block{ syntax };
	block.bind();
	block.dispose();
}

void Typer::bindForInStatement(ForInSyntax *syntax) {
	tp_forin_loop loop{ syntax };
	loop.bind();
	loop.dispose();
}

void Typer::bindForStatement(ForSyntax *syntax) {
	tp_for_loop loop{ syntax };
	loop.bind();
	loop.dispose();
}

void Typer::bindWhileStatement(WhileSyntax *syntax) {
	tp_while_loop loop{ syntax };
	loop.bind();
	loop.dispose();
}
} // namespace exy