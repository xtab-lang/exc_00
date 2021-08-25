#include "pch.h"
#include "typer.h"

namespace exy {
TpTypeNode* tp_isa::TypeNode(TpNode *node) {
	if (node != nullptr) {
		switch (node->kind) {
		#define ZM(zName) case TpKind::zName: return (TpTypeNode*)node;
			DeclareTpTypeSymbolNodes(ZM)
			#undef ZM
		}
	}
	return nullptr;
}
TpSymbol* tp_isa::TypeSymbol(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return TypeNode(symbol->node) ? symbol : nullptr;
	}
	return nullptr;
}

TpAliasNode* tp_isa::AliasNode(TpNode *node) {
	if (node != nullptr) {
		switch (node->kind) {
		#define ZM(zName) case TpKind::zName: return (TpAliasNode*)node;
			DeclareTpAliasSymbolNodes(ZM)
			#undef ZM
		}
	}
	return nullptr;
}

TpSymbol* tp_isa::AliasSymbol(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return AliasNode(symbol->node) ? symbol : nullptr;
	}
	return nullptr;
}

TpValueNode* tp_isa::ValueNode(TpNode *node) {
	if (node != nullptr) {
		switch (node->kind) {
		#define ZM(zName) case TpKind::zName: return (TpValueNode*)node;;
			DeclareTpValueSymbolNodes(ZM)
			#undef ZM
		}
	}
	return nullptr;
}

TpSymbol* tp_isa::ValueSymbol(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return ValueNode(symbol->node) ? symbol : nullptr;
	}
	return nullptr;
}

TpSymbol* tp_isa::ModuleScope(TpScope *scope) {
	if (scope != nullptr) {
		return ModuleSymbol(scope->owner);
	}
	return nullptr;
}

TpConstExpr* tp_isa::Null(TpNode *node) {
	if (auto  expr = ConstExpr(node)) {
		auto value = (TpLiteral*)expr->value;
		return value->kind == TpKind::Literal && value->type.isVoidPointer() && value->u64 == 0ui64 ? expr : nullptr;
	}
	return nullptr;
}

TpSymbol* tp_isa::ResumableStruct(Type type) {
	if (auto ptr = type.isIndirect()) {
		const auto &pointee = ptr->pointee;
		if (auto stSymbol = pointee.isaStruct()) {
			auto   stNode = (TpStruct*)stSymbol->node;
			return stNode->isaResumableStruct() ? stSymbol : nullptr;
		}
	} else if (auto stSymbol = type.isaStruct()) {
		auto   stNode = (TpStruct*)stSymbol->node;
		return stNode->isaResumableStruct() ? stSymbol : nullptr;
	}
	return nullptr;
}

bool tp_isa::PassByValue(Type type) {
	if (type.isaBuiltin() || type.isIndirect()) {
		return true;
	}
	if (type.isaFunctionOrFunctionTemplate()) {
		return true;
	}
	if (auto enumSymbol = type.isanEnum()) {
		auto   enumNode = (TpEnum*)enumSymbol->node;
		return PassByValue(enumNode->valueType);
	}
	return false;
}

TpSymbol* tp_isa::LambdaStructSymbol(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return LambdaStruct(symbol->node) ? symbol : nullptr;
	}
	return nullptr;
}

TpStruct* tp_isa::LambdaStruct(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return LambdaStruct(symbol->node);
	}
	return nullptr;
}

TpStruct* tp_isa::LambdaStruct(TpNode *node) {
	if (node != nullptr) {
		if (node->kind == TpKind::Struct) {
			auto st = (TpStruct*)node;
			return st->structKind == TpStruct::LambdaStruct ? st : nullptr;
		}
	}
	return nullptr;
}

TpSymbol* tp_isa::LambdaFunctionSymbol(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return LambdaFunction(symbol->node) ? symbol : nullptr;
	}
	return nullptr;
}

TpFunction* tp_isa::LambdaFunction(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return LambdaFunction(symbol->node);
	}
	return nullptr;
}

TpFunction* tp_isa::LambdaFunction(TpNode *node) {
	if (node != nullptr) {
		if (node->kind == TpKind::Function) {
			auto fn = (TpFunction*)node;
			return fn->isaLambdaFunction() ? fn : nullptr;
		}
	}
	return nullptr;
}

TpSymbol* tp_isa::LambdaTemplateSymbol(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return LambdaTemplate(symbol->node) ? symbol : nullptr;
	}
	return nullptr;
}

TpTemplate* tp_isa::LambdaTemplate(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return LambdaTemplate(symbol->node);
	}
	return nullptr;
}

TpTemplate* tp_isa::LambdaTemplate(TpNode *node) {
	if (node != nullptr) {
		if (node->kind == TpKind::Template) {
			auto t = (TpTemplate*)node;
			if (t->syntax->pos.keyword == Keyword::Lambda) {
				return t;
			}
		}
	}
	return nullptr;
}


TpSymbol* tp_isa::LocalOrParameterSymbol(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return LocalOrParameter(symbol->node) ? symbol : nullptr;
	}
	return nullptr;
}

TpValueNode* tp_isa::LocalOrParameter(TpSymbol *symbol) {
	if (symbol != nullptr) {
		return LocalOrParameter(symbol->node);
	}
	return nullptr;
}

TpValueNode* tp_isa::LocalOrParameter(TpNode *node) {
	if (node != nullptr) {
		return node->kind == TpKind::Local || node->kind == TpKind::Parameter ? (TpValueNode*)node : nullptr;
	}
	return nullptr;
}

TpName* tp_isa::Name(TpNode *node) {
	if (node != nullptr) {
		switch (node->kind) {
		#define ZM(zName) case TpKind::zName: return (TpName*)node;
			DeclareTpNameNodes(ZM)
			#undef ZM
		}
	}
	return nullptr;
}

TpFunction* tp_isa::MemberFunction(TpFunction *node) {
	if (node != nullptr) {
		if (node->parameters.isNotEmpty()) {
			auto firstParameter = node->parameters.first();
			if (firstParameter->name == ids.kw_this) {
				Assert(!node->modifiers.isStatic);
				return node;
			}
			Assert(node->modifiers.isStatic);
		}
	}
	return nullptr;
}

TpTemplate* tp_isa::FunctionTemplate(TpSymbol *symbol) {
	if (auto t = Template(symbol)) {
		return FunctionSyntax(t->syntax) ? t : nullptr;
	}
	return nullptr;
}

TpValueName* tp_isa::This(TpNode *node) {
	if (node != nullptr && node->kind == TpKind::ValueName) {
		auto   name = (TpValueName*)node;
		auto symbol = name->symbol;
		if (symbol->name == ids.kw_this) {
			return name;
		}
	}
	return nullptr;
}

SyntaxNode* tp_isa::BlockMaker(SyntaxNode *node) {
	if (node != nullptr) {
		switch (node->kind) {
			case SyntaxKind::Block:
			case SyntaxKind::If:
			case SyntaxKind::Switch:
			case SyntaxKind::Case:
			case SyntaxKind::ForIn:
			case SyntaxKind::For:
			case SyntaxKind::While:
			case SyntaxKind::DoWhile:
			case SyntaxKind::Using:
				return node;
		}
	}
	return nullptr;
}

} // namespace exy