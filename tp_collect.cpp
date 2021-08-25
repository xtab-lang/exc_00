#include "pch.h"
#include "typer.h"

#define template_error(pos, msg, ...) diagnostic("Template", pos, msg, __VA_ARGS__)

namespace exy {
static Identifier mkTemplateName(SyntaxNode *name, const String &prefix) {
	auto &tp = *typer;
	if (name == nullptr) {
		return ids.random(prefix.text, prefix.length);
	}
	if (auto id = tp.isa.IdentifierSyntax(name)) {
		return id->value;
	}
	if (auto id = tp.isa.TextSyntax(name)) {
		return ids.get(id->value);
	}
	if (auto id = tp.isa.DoubleQuotedSyntax(name)) {
		return ids.get(id->value);
	}
	if (auto id = tp.isa.SingleQuotedSyntax(name)) {
		return ids.get(id->value);
	}
	if (auto tpname = tp.isa.TypeNameSyntax(name)) {
		if (auto id = tp.isa.IdentifierSyntax(tpname->name)) {
			return id->value;
		}
		return ids.random(prefix.text, prefix.length);
	}
	template_error(name, "expected an identifier, text or typename, not %sk", name->kind);
	return nullptr;
}

static TpArity mkTemplateParameters(AngledSyntax *node) {
	auto &tp = *typer;
	auto required = 0;
	auto default_ = 0;
	auto   varags = false;
	if (node == nullptr || node->value == nullptr) {
		// Do nothing.
	} else if (auto list = tp.isa.CommaSeparatedSyntax(node->value)) {
		for (auto i = 0; i < list->nodes.length; i++) {
			auto param = list->nodes.items[i];
			if (tp.isa.NameValueSyntax(param)) {
				if (varags) {
					template_error(param, "default type parameter after vararg");
				} else {
					++default_;
				}
			} else if (tp.isa.RestParameterSyntax(param)) {
				if (varags) {
					template_error(param, "multiple varag type parameters");
				} else {
					varags = true;
				}
			} else if (default_ > 0 || varags) {
				template_error(param, "required type parameter after either default or vararg");
			} else if (tp.isa.IdentifierSyntax(param)) {
				++required;
			} else {
				template_error(param, "expected either identifier, name-value or vararg as a type parameter, not %sk", param->kind);
			}
		}
	} else if (tp.isa.NameValueSyntax(node->value)) {
		++default_;
	} else if (tp.isa.RestParameterSyntax(node->value)) {
		varags = true;
	} else if (tp.isa.IdentifierSyntax(node->value)) {
		++required;
	} else if (node->value != nullptr) {
		template_error(node->value, "expected either identifier, name-value or vararg");
	}
	return { required, default_, varags, /* hasThis = */ false };
}

static IdentifierSyntax* getVariableName(VariableSyntax *syntax) {
	auto &tp = *typer;
	if (auto nv = tp.isa.NameValueSyntax(syntax->name)) {
		return nv->name;
	}
	return tp.isa.IdentifierSyntax(syntax->name);
}

static TpArity mkTemplateParameters(ParenthesizedSyntax *node) {
	auto &tp = *typer;
	auto required = 0;
	auto default_ = 0;
	auto   varags = false;
	auto  hasThis = false;
	if (node == nullptr || node->value == nullptr) {
		// Do nothing.
	} else if (auto list = tp.isa.CommaSeparatedSyntax(node->value)) {
		for (auto i = 0; i < list->nodes.length; i++) {
			auto  isFirst = i == 0;
			auto    param = list->nodes.items[i];
			if (auto  var = tp.isa.VariableSyntax(param)) {
				auto name = getVariableName(var);
				if (varags) {
					template_error(param, "required or default function parameter after vararg");
				} else if (var->rhs != nullptr) {
					++default_;
					if (name->value == ids.kw_this) {
						template_error(var->name, "invalid name for a default parameter: %s#<red>", name->value);
					}
				} else if (default_ > 0) {
					template_error(param, "required function parameter after default function parameter");
				} else {
					++required;
					if (isFirst && name->value == ids.kw_this) {
						hasThis = true;
					}
				}
			} else if (auto rest = tp.isa.RestParameterSyntax(param)) {
				if (auto name = rest->name) {
					if (name->value == ids.kw_this) {
						template_error(rest->name, "invalid name for a vararg parameter: %s#<red>",
									 name->value);
					}
				}
				if (varags) {
					template_error(param, "multiple vararg parameters");
				} else {
					varags = true;
				}
			} else {
				template_error(param, "expected either a function-parameter or vararg");
			}
		}
	} else if (auto var = tp.isa.VariableSyntax(node->value)) {
		auto name = getVariableName(var);
		if (var->rhs != nullptr) {
			++default_;
			if (name->value == ids.kw_this) {
				template_error(var->name, "invalid name for a default parameter: %s#<red>", name->value);
			}
		} else {
			++required;
			if (name->value == ids.kw_this) {
				hasThis = true;
			}
		}
	} else if (auto rest = tp.isa.RestParameterSyntax(node->value)) {
		varags = true;
		if (auto name = rest->name) {
			if (name->value == ids.kw_this) {
				template_error(rest->name, "invalid name for a vararg parameter: %s#<red>",
							 name->value);
			}
		}
	} else if (node->value != nullptr) {
		template_error(node->value, "expected either a function parameter or vararg");
	}
	return { required, default_, varags, hasThis };
}
static auto canCreateOverloadedFrom(TpSymbol *found, Keyword keyword,
									const TpArity &parameters) {
	if (typer->isa.TemplateSymbol(found)) {
		auto node = (TpTemplate*)found->node;
		if (node->syntax->pos.keyword != keyword) {
			return false;
		}
		return node->syntax->pos.keyword == keyword &&
			node->arity.required != parameters.required;
	}
	if (typer->isa.OverloadSetSymbol(found)) {
		auto ov = (TpOverloadSet*)found->node;
		for (auto i = 0; i < ov->list.length; i++) {
			auto node = (TpTemplate*)ov->list.items[i]->node;
			if (node->syntax->pos.keyword != keyword) {
				return false;
			}
			if (node->arity.required == parameters.required) {
				return false;
			}
		}
		return true;
	}
	return false;
}

//----------------------------------------------------------
void Typer::collectTemplates(SyntaxNodes statements) {
	for (auto i = 0; i < statements.length; i++) {
		auto syntax = statements.items[i];
		collectTemplate(syntax);
	}
}

TpSymbol* Typer::collectTemplate(SyntaxNode *syntax) {
	const auto errors = compiler.errors;
	TpSymbol  *symbol = nullptr;
	switch (syntax->kind) {
		case SyntaxKind::ExternBlock: {
			auto block = (ExternBlockSyntax*)syntax;
			auto  prev = current->pushExternBlock(block);
			collectTemplates(block->nodes);
			current->popExternBlock(prev, block);
		} break;
		case SyntaxKind::Block: if (isa.ModuleScope(current->scope)) {
			auto block = (BlockSyntax*)syntax;
			current->pushBlockModifiers(block->modifiers);
			collectTemplates(block->nodes);
			current->popBlockModifiers(block->modifiers);
		} break;
		case SyntaxKind::Structure: {
			auto   node = (StructureSyntax*)syntax;
			auto prefix = node->pos.sourceValue();
			if (auto  name = mkTemplateName(node->name, prefix)) {
				auto arity = mkTemplateParameters(node->parameters);
				if (errors != compiler.errors) {
					break;
				}
				if (auto found = current->scope->contains(name)) {
					if (canCreateOverloadedFrom(found, node->pos.keyword, arity)) {
						if (isa.TemplateSymbol(found)) {
							auto idx = current->scope->symbols.indexOf(name);
							Assert(current->scope->symbols.items[idx].value == found);
							symbol = mk.OverloadSet(found);
							current->scope->symbols.items[idx].value = symbol;
						} else if (isa.OverloadSetSymbol(found)) {
							symbol = found;
						} else {
							Assert(0);
						}
						symbol = mk.Template(symbol, node, arity, name);
					} else {
						dup_error(node, found);
					}
				} else {
					symbol = mk.Template(node, arity, name);
				}
			}
		} break;
		case SyntaxKind::Function: {
			auto node = (FunctionSyntax*)syntax;
			if (isa.kwUrlHandler(node->pos.keyword)) {
				return collectUrlHandler(node);
			}
			if (isa.kwExtern(node->pos.keyword)) {
				return collectExtern(node);
			}
			auto prefix = SourceToken::value(node->pos.keyword); // node->pos.sourceValue();
			if (auto  name = mkTemplateName(node->name, prefix)) {
				auto arity = mkTemplateParameters(node->parameters);
				if (errors != compiler.errors) {
					break;
				}
				if (auto found = current->scope->contains(name)) {
					if (canCreateOverloadedFrom(found, node->pos.keyword, arity)) {
						if (isa.TemplateSymbol(found)) {
							auto idx = current->scope->symbols.indexOf(name);
							Assert(current->scope->symbols.items[idx].value == found);
							symbol = mk.OverloadSet(found);
							current->scope->symbols.items[idx].value = symbol;
						} else if (isa.OverloadSetSymbol(found)) {
							symbol = found;
						} else {
							Assert(0);
						}
						symbol = mk.Template(symbol, node, arity, name);
					} else {
						dup_error(node, found);
					}
				} else {
					symbol = mk.Template(node, arity, name);
				}
			}
		} break;
	}
	return symbol;
}

TpSymbol* Typer::collectUrlHandler(FunctionSyntax *syntax) {
	if (!isa.ModuleScope(current->scope)) {
		template_error(syntax, "urlhandlers must be declared in file scope");
		return nullptr;
	}
	const auto errors = compiler.errors;
	TpSymbol  *symbol = nullptr;
	auto        scope = getParentModuleScopeOf();
	auto       prefix = syntax->pos.sourceValue();
	if (auto  name = mkTemplateName(syntax->name, prefix)) {
		auto arity = mkTemplateParameters(syntax->parameters);
		if (errors != compiler.errors) {
			return nullptr;
		}
		symbol = mk.UrlHandlerTemplate(scope, syntax, arity, name);
	}
	return symbol;
}

//---
static void buildDllNameFrom(String &s, SyntaxNode *syntax) {
	auto &tp = *typer;
	if (auto dot = tp.isa.DotSyntax(syntax)) {
		buildDllNameFrom(s, dot->lhs);
		buildDllNameFrom(s, dot->rhs);
	} else if (auto id = tp.isa.IdentifierSyntax(syntax)) {
		if (s.isNotEmpty()) {
			s.append(S("."));
		}
		s.append(id->value);
	} else {
		template_error(syntax, "expected an identifier in dll name");
	}
}
//---

TpSymbol* Typer::collectExtern(FunctionSyntax *syntax) {
	if (current->ext == nullptr) {
		template_error(syntax, "%kw functions must be declared in an %sk block: %kw <dllname> %kw { ... }", 
					   syntax->pos.keyword, SyntaxKind::ExternBlock, Keyword::From, Keyword::Import);
		return nullptr;
	}
	if (syntax->name == nullptr) {
		template_error(syntax, "%kw functions must be declared with a name",
					   syntax->pos.keyword);
	}
	Identifier fnName = nullptr;
	if (auto id = isa.IdentifierSyntax(syntax->name)) {
		fnName = id->value;
	} else {
		template_error(syntax->name, "%kw functions must be declared with an identifier as name, not %sk",
					   syntax->name->kind);
		return nullptr;
	}
	String tmp{};
	buildDllNameFrom(tmp, current->ext->name);
	auto dllName = ids.get(tmp);
	tmp.dispose();
	//--
	Identifier dllPath = nullptr;
	if (auto hModule = LoadLibrary(dllName->text)) {
		if (GetProcAddress(hModule, fnName->text) != nullptr) {
			auto len = GetModuleFileName(hModule, tmpbuf, tmpbufcap);
			if (len == 0) {
				template_error(syntax, "'GetModuleFileName' for %s#<red> in %s#<red> failed",
							   fnName, dllName);
			} else if (len == tmpbufcap) {
				if (GetLastError() == ERROR_SUCCESS) {
					dllPath = ids.get(tmpbuf, len);
				} else {
					template_error(syntax, "'GetModuleFileName' for %s#<red> in %s#<red> failed because the path is too long",
								   fnName, dllName);
				}
			} else if (GetLastError() == ERROR_SUCCESS) {
				dllPath = ids.get(tmpbuf, len);
			} else {
				template_error(syntax, "'GetProcAddress' with %s#<red> failed for %s#<red> because of %i#0x",
							   fnName, dllName, GetLastError());
			}
		} else {
			template_error(syntax, "'GetProcAddress' with %s#<red> failed for %s#<red> because of %i#0x", 
						   fnName, dllName, GetLastError());
		}
		FreeLibrary(hModule);
	} else {
		template_error(syntax, "'LoadLibrary' with %s#<red> failed for %s#<red>", dllName, fnName);
	}
	if (dllPath != nullptr) {
		const auto errors = compiler.errors;
		auto       prefix = syntax->pos.sourceValue();
		if (auto  name = mkTemplateName(syntax->name, prefix)) {
			auto arity = mkTemplateParameters(syntax->parameters);
			if (errors == compiler.errors) {
				return mk.ExternTemplate(syntax, arity, name, dllPath);
			}
		}
	}
	return nullptr;
}
} // namespace exy