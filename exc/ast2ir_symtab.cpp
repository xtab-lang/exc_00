//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-29
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ast2ir.h"

#define err(token, msg, ...) print_error("Ast2IrSymbolTable", token, msg, __VA_ARGS__)

namespace exy {
namespace ast2ir_pass {
void SymbolTable::dispose() {
	list.dispose();
}

IrBuiltin* SymbolTable::append(IrModule *parent, AstBuiltin *ast) {
	auto name = fullNameOf(ast);
	auto &mem = comp.ir->mem;
	auto   ir = mem.New<IrBuiltin>(ast->loc, parent->data, name, ast->keyword);
	list.append(UINT64(ast), { parent, ast, ir });
	return ir;
}

IrModule* SymbolTable::append(AstModule *ast) {
	auto &mem = comp.ir->mem;
	auto   ir = mem.New<IrModule>(ast->loc, ast->dotName, ast->binaryKind);
	list.append(UINT64(ast), { nullptr, ast, ir });
	return ir;
}

IrGlobal* SymbolTable::append(IrModule *parent, AstGlobal *ast, const IrType &type) {
	auto name = fullNameOf(ast);
	auto &mem = comp.ir->mem;
	auto   ir = mem.New<IrGlobal>(ast->loc, parent->data, type, name);
	list.append(UINT64(ast), { parent, ast, ir });
	return ir;
}

IrSymbol* SymbolTable::get(AstSymbol *ast) {
	auto idx = list.indexOf(UINT64(ast));
	if (idx >= 0) {
		return list.items[idx].value.ir;
	}
	Unreachable();
}

IrSymbol* SymbolTable::find(AstSymbol *ast) {
	auto idx = list.indexOf(UINT64(ast));
	if (idx >= 0) {
		return list.items[idx].value.ir;
	}
	return nullptr;
}

Identifier SymbolTable::fullNameOf(AstSymbol *ast) {
	List<Identifier> names{};
	names.append(ast->name);
	for (auto scope = ast->parentScope; scope; scope = scope->parent) {
		names.append(scope->owner->name);
	}
	String name{};
	for (auto i = names.length - 1; i >= 0; --i) {
		if (name.length) name.append(S("."));
		name.append(names.items[i]);
	}
	auto id = ids.get(name);
	name.dispose();
	names.dispose();
	return id;
}

//------------------------------------------------------------------------------------------------
void SymbolTableBuilder::visitTree() {
	auto &tree = *comp.ast;
	visitModule(tree.global);
}

void SymbolTableBuilder::visitSymbols(Dict<AstSymbol*> &list) {
	for (auto i = 0; i < list.length; ++i) {
		auto ast = list.items[i].value;
		visitSymbol(ast);
	}
}

void SymbolTableBuilder::visitSymbols(List<AstSymbol*> &list) {
	for (auto i = 0; i < list.length; ++i) {
		auto ast = list.items[i];
		visitSymbol(ast);
	}
}

void SymbolTableBuilder::visitScope(AstScope *scope) {
	visitSymbols(scope->others);
	visitSymbols(scope->symbols);
}

IrSymbol* SymbolTableBuilder::visitSymbol(AstSymbol *ast) {
	if (auto found = symtab.find(ast)) {
		return found;
	} 
	
	if (auto scope = ast->ownScope) {
		if (scope->status.isIdle()) {
			return nullptr;
		}
		Assert(scope->status.isDone());
	}

	switch (ast->kind) {
		case AstKind::Builtin:
			return visitBuiltin((AstBuiltin*)ast);

		case AstKind::Module:
			return visitModule((AstModule*)ast);

		case AstKind::TypeAlias:
		case AstKind::ValueAlias:
		case AstKind::ConstAlias:
			break; // Do nothing.

		case AstKind::Global:
			return visitGlobal((AstGlobal*)ast);

		default: {
			err(ast, "%ast not implemented", ast->kind);
		} break;
	}
	return nullptr;
}

IrBuiltin* SymbolTableBuilder::visitBuiltin(AstBuiltin *ast) {
	auto ir = symtab.append(moduleOf(ast), ast);
	switch (ast->keyword) {
	#define ZM(zName, zSize) case Keyword::zName: comp.ir->ty##zName = { ir }; break;
		DeclareBuiltinTypeKeywords(ZM)
	#undef ZM
		default: Unreachable();
	}
	return ir;
}

IrModule* SymbolTableBuilder::visitModule(AstModule *ast) {
	auto ir = symtab.append(ast);
	ir->entry = mem.New<IrFunction>(ast->loc, ir->code, ast->name, typeOf(comp.ast->tyVoid));
	visitScope(ast->ownScope);
	return ir;
}

IrGlobal* SymbolTableBuilder::visitGlobal(AstGlobal *ast) {
	return symtab.append(moduleOf(ast), ast, typeOf(ast->type));
}

IrType SymbolTableBuilder::typeOf(const AstType &ast) {
	if (auto ptr = ast.isaPointer()) {
		return typeOf(ptr->pointee).pointer();
	} if (auto ref = ast.isaReference()) {
		return typeOf(ref->pointee).pointer();
	} if (auto  sym = ast.isDirect()) {
		if (auto ir = visitSymbol(sym)) {
			return ir->type;
		}
	}
	Unreachable();
}

IrModule* SymbolTableBuilder::moduleOf(AstSymbol *ast) {	;
	for (auto scope = ast->parentScope; scope; scope = scope->parent) {
		if (scope->owner->kind == AstKind::Module) {
			return (IrModule*)visitSymbol(scope->owner);
		}
	}
	Unreachable();
}
} // namespace ast2ir_pass
} // namespace exy