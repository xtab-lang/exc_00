#include "pch.h"
#include "typer.h"

namespace exy {
//----------------------------------------------------------
tp_mk::tp_mk(Typer *t) 
    : tp(*t), mem(t->mem) {}

TpSymbol* tp_mk::TypeAlias(Pos pos, Identifier name, Type type, TpAliasKind kind) {
    switch (kind) {
        case TpAliasKind::Define:
            return DefineTypeAlias(pos, name, type);
        case TpAliasKind::Import:
            return ImportTypeAlias(pos, name, type);
        case TpAliasKind::Export:
            return ExportTypeAlias(pos, name, type);
    }
    impl_error(pos, "mk.TypeAlias()");
    return nullptr;
}

TpSymbol* tp_mk::DefineTypeAlias(Pos pos, Identifier name, Type type) {
    auto node = mem.New<TpTypeAlias>(tp.mkPos(pos), type, dotName(name), TpAliasKind::Define);
    return Symbol(name, node);
}

TpSymbol* tp_mk::ImportTypeAlias(Pos pos, Identifier name, Type type) {
    auto node = mem.New<TpTypeAlias>(tp.mkPos(pos), type, dotName(name), TpAliasKind::Import);
    return Symbol(name, node);
}

TpSymbol* tp_mk::ExportTypeAlias(Pos pos, Identifier name, Type type) {
    auto node = mem.New<TpTypeAlias>(tp.mkPos(pos), type, dotName(name), TpAliasKind::Export);
    return Symbol(name, node);
}

//----------------------------------------------------------
TpSymbol* tp_mk::ConstAlias(Pos pos, Identifier name, TpConstExpr *value, TpAliasKind kind) {
    switch (kind) {
        case TpAliasKind::Define:
            return DefineConstAlias(pos, name, value);
        case TpAliasKind::Import:
            return ImportConstAlias(pos, name, value);
        case TpAliasKind::Export:
            return ExportConstAlias(pos, name, value);
    }
    impl_error(pos, "mk.TypeAlias()");
    tp.throwAway(value);
    return nullptr;
}

TpSymbol* tp_mk::DefineConstAlias(Pos pos, Identifier name, TpConstExpr *value) {
    auto node = mem.New<TpConstAlias>(tp.mkPos(pos), value, dotName(name), TpAliasKind::Define);
    return Symbol(name, node);
}

TpSymbol* tp_mk::ImportConstAlias(Pos pos, Identifier name, TpConstExpr *value) {
    auto node = mem.New<TpConstAlias>(tp.mkPos(pos), value, dotName(name), TpAliasKind::Import);
    return Symbol(name, node);
}

TpSymbol* tp_mk::ExportConstAlias(Pos pos, Identifier name, TpConstExpr *value) {
    auto node = mem.New<TpConstAlias>(tp.mkPos(pos), value, dotName(name), TpAliasKind::Export);
    return Symbol(name, node);
}

//----------------------------------------------------------
TpSymbol* tp_mk::ValueAlias(Pos pos, Identifier name, TpSymbol *value, TpAliasKind kind) {
    switch (kind) {
        case TpAliasKind::Define:
            return DefineValueAlias(pos, name, value);
        case TpAliasKind::Import:
            return ImportValueAlias(pos, name, value);
        case TpAliasKind::Export:
            return ExportValueAlias(pos, name, value);
    }
    impl_error(pos, "%c", __FUNCTION__);
    return nullptr;
}

TpSymbol* tp_mk::DefineValueAlias(Pos pos, Identifier name, TpSymbol *value) {
    Assert(tp.isa.ValueSymbol(value));
    auto node = mem.New<TpValueAlias>(tp.mkPos(pos), value, dotName(name), TpAliasKind::Define);
    return Symbol(name, node);
}

TpSymbol* tp_mk::ImportValueAlias(Pos pos, Identifier name, TpSymbol *value) {
    Assert(tp.isa.ValueSymbol(value));
    auto node = mem.New<TpValueAlias>(tp.mkPos(pos), value, dotName(name), TpAliasKind::Import);
    return Symbol(name, node);
}

TpSymbol* tp_mk::ExportValueAlias(Pos pos, Identifier name, TpSymbol *value) {
    Assert(tp.isa.ValueSymbol(value));
    auto node = mem.New<TpValueAlias>(tp.mkPos(pos), value, dotName(name), TpAliasKind::Export);
    return Symbol(name, node);
}

TpSymbol* tp_mk::Global(Pos pos, Identifier name, Type type) {
    auto node = mem.New<TpGlobal>(tp.mkPos(pos), type, dotName(name));
    node->modifiers.isStatic = true;
    return Symbol(name, node);
}

TpSymbol* tp_mk::Global(Pos pos, Identifier name, TpNode *rhs) {
    auto node = mem.New<TpGlobal>(tp.mkPos(pos), rhs, dotName(name));
    node->modifiers.isStatic = true;
    return Symbol(name, node);
}

TpSymbol* tp_mk::Field(Pos pos, Identifier name, Type type) {
    auto node = mem.New<TpField>(tp.mkPos(pos), type, dotName(name), TpField::OrdinaryField);
    return Symbol(name, node);
}

TpSymbol* tp_mk::Field(Pos pos, Identifier name, TpNode *rhs) {
    auto node = mem.New<TpField>(tp.mkPos(pos), rhs, dotName(name), TpField::OrdinaryField);
    return Symbol(name, node);
}

TpSymbol* tp_mk::CapturedField(Pos pos, TpScope *scope, Identifier name, TpNode *rhs) {
    auto node = mem.New<TpField>(tp.mkPos(pos), rhs, dotName(name), TpField::CaptureField);
    return Symbol(scope, name, node);
}

TpSymbol* tp_mk::Local(Pos pos, Identifier name, Type type) {
    if (name == ids.kw_blank) {
        name = ids.random(name);
    }
    auto node = mem.New<TpLocal>(tp.mkPos(pos), type, dotName(name));
    return Symbol(name, node);
}

TpSymbol* tp_mk::Parameter(Pos pos, Identifier name, Type type) {
    auto node = mem.New<TpParameter>(tp.mkPos(pos), type, dotName(name));
    return Symbol(name, node);
}

TpSymbol* tp_mk::OverloadSet(TpSymbol *first) {
    Assert(first->scope == tp.current->scope);
    auto   node = mem.New<TpOverloadSet>(first);
    auto symbol = mem.New<TpSymbol>(first->scope, first->name, node);
    node->type = symbol;
    return symbol;
}

static void setAsyncOrGenerator(TpSymbol *templateSymbol, FunctionSyntax *syntax) {
    auto fnNode = (TpTemplate*)templateSymbol->node;
    if (syntax->awaits > 0) {
        fnNode->modifiers.isAsync = true;
    }
    if (syntax->yields) {
        fnNode->modifiers.isaGenerator = true;
    }
    auto &tp = *typer;
    auto isResumable = fnNode->modifiers.isResumable();
    auto  hasReturns = syntax->returns != 0;
    if (syntax->pos.keyword == Keyword::Lambda) {
        if (auto modifiers = syntax->modifiers) {
            syntax_error(modifiers, "no modifiers allowed on %tptype", &fnNode->type);
        }
        if (isResumable) {
            syntax_error(syntax, "a %kw cannot be a generator: %tptype", Keyword::Lambda, &fnNode->type);
        }
        if (fnNode->arity.hasThis) {
            syntax_error(syntax, "%s#<red> parameter is not allowed on %tptype", ids.kw_this,
                         &fnNode->type);
        }
    } else if (auto stSymbol = tp.isa.StructSymbol(templateSymbol->scope->owner)) {
        if (stSymbol->name == templateSymbol->name) { // Constructor
            if (syntax->pos.keyword != Keyword::Fn) {
                syntax_error(syntax, "a %kw cannot be a constructor: %tptype", 
                             syntax->pos.keyword, &fnNode->type);
            }
            if (auto modifiers = syntax->modifiers) {
                syntax_error(modifiers, "no modifiers allowed on constructor %tptype", &fnNode->type);
            }
            if (isResumable) {
                syntax_error(syntax, "a constructor cannot be a generator: %tptype", &fnNode->type);
            }
            if (hasReturns) {
                syntax_error(syntax, "a constructor cannot have return statements: %tptype", &fnNode->type);
            }
        } else if (templateSymbol->name == ids.kw_dispose) { // Disposer
            if (syntax->pos.keyword != Keyword::Fn) {
                syntax_error(syntax, "a %kw cannot be a disposer: %tptype",
                             syntax->pos.keyword, &fnNode->type);
            }
            if (auto modifiers = syntax->modifiers) {
                syntax_error(modifiers, "no modifiers allowed on disposer %tptype", &fnNode->type);
            }
            if (isResumable) {
                syntax_error(syntax, "a disposer cannot be a generator: %tptype", &fnNode->type);
            }
            if (hasReturns) {
                syntax_error(syntax, "a disposer cannot have return statements: %tptype", &fnNode->type);
            }
        }
    }
    if (!fnNode->arity.hasThis) {
        fnNode->modifiers.isStatic = true;
    }
}

TpSymbol* tp_mk::Template(TpSymbol *ovSymbol, StructureSyntax *syntax, const TpArity &arity, Identifier name) {
    Assert(ovSymbol->scope == tp.current->scope);
    auto     ov = (TpOverloadSet*)ovSymbol->node;
    auto   node = mem.New<TpTemplate>(syntax, arity, dotName(name));
    auto symbol = mem.New<TpSymbol>(ovSymbol->scope, name, node);
    node->type = symbol;
    node->parentOv = ov;
    ov->list.append(symbol);
    tp.applyModifiers(syntax->modifiers, symbol);
    return symbol;
}

TpSymbol* tp_mk::Template(TpSymbol *ovSymbol, FunctionSyntax *syntax, const TpArity &arity, Identifier name) {
    Assert(ovSymbol->scope == tp.current->scope);
    auto     ov = (TpOverloadSet*)ovSymbol->node;
    auto   node = mem.New<TpTemplate>(syntax, arity, dotName(name));
    auto symbol = mem.New<TpSymbol>(ovSymbol->scope, name, node);
    node->type = symbol;
    node->parentOv = ov;
    ov->list.append(symbol);
    tp.applyModifiers(syntax->modifiers, symbol);
    setAsyncOrGenerator(symbol, syntax);
    return symbol;
}

TpSymbol* tp_mk::Template(StructureSyntax *syntax, const TpArity &arity, Identifier name) {
    auto   node = mem.New<TpTemplate>(syntax, arity, dotName(name));
    auto symbol = Symbol(name, node);
    node->type = symbol;
    tp.applyModifiers(syntax->modifiers, symbol);
    return symbol;
}

TpSymbol* tp_mk::Template(FunctionSyntax *syntax, const TpArity &arity, Identifier name) {
    auto   node = mem.New<TpTemplate>(syntax, arity, dotName(name));
    auto symbol = Symbol(name, node);
    node->type = symbol;
    tp.applyModifiers(syntax->modifiers, symbol);
    setAsyncOrGenerator(symbol, syntax);
    return symbol;
}

TpSymbol* tp_mk::UrlHandlerTemplate(TpScope *moduleScope, FunctionSyntax *syntax, const TpArity &arity,
                                    Identifier name) {
    auto   node = mem.New<TpTemplate>(syntax, arity, dotName(name));
    auto symbol = mem.New<TpSymbol>(moduleScope, name, node);
    node->type = symbol;
    tp.applyModifiers(syntax->modifiers, symbol);
    auto tpModule = (TpModule*)moduleScope->owner->node;
    tpModule->urlHandlers.append(symbol);
    return symbol;
}

TpSymbol * tp_mk::ExternTemplate(FunctionSyntax *syntax, const TpArity &arity, Identifier name, 
                                 Identifier dllPath) {
    auto   node = mem.New<TpTemplate>(syntax, arity, dotName(name));
    auto symbol = Symbol(name, node);
    node->type = symbol;
    node->dllPath = dllPath;
    tp.applyModifiers(syntax->modifiers, symbol);
    return symbol;
}

static auto indexOfTemplateInOverload(TpOverloadSet *parentOv, TpSymbol *templateSymbol) {
    for (auto i = 0; i < parentOv->list.length; i++) {
        if (parentOv->list.items[i] == templateSymbol) {
            return i;
        }
    }
    UNREACHABLE();
}

static auto indexOfTemplateInScope(TpSymbol *templateSymbol) {
    auto scope = templateSymbol->scope;
    auto   idx = scope->symbols.indexOf(templateSymbol->name);
    if (idx >= 0) {
        Assert(scope->symbols.items[idx].value == templateSymbol);
        return idx;
    }
    UNREACHABLE();
}

static auto exchangeTemplateSymbolWithInstanceSymbol(TpSymbol *templateSymbol, TpSymbol *instanceSymbol) {
    auto templateNode = (TpTemplate*)templateSymbol->node;
    auto instanceNode = instanceSymbol->node;
    auto        scope = templateSymbol->scope;
    if (auto parentOv = templateNode->parentOv) {
        auto      idx = indexOfTemplateInOverload(parentOv, templateSymbol);
        templateNode->instances.append(templateSymbol);
        parentOv->list.items[idx] = instanceSymbol;
    } else {
        auto   idx = indexOfTemplateInScope(templateSymbol);
        templateNode->instances.append(templateSymbol);
        scope->symbols.items[idx].value = instanceSymbol;
    }

    templateSymbol->node = instanceNode;
    instanceNode->type = templateSymbol;

    instanceSymbol->node = templateNode;
    templateNode->type = instanceSymbol;

    auto &mem = compiler.tpTree->mem;
    auto node = (TpTypeNode*)instanceNode;
    node->scope = mem.New<TpScope>(scope, templateSymbol);

    return tp_template_instance_pair{ instanceSymbol, templateSymbol };
}

tp_template_instance_pair tp_mk::Struct(TpSymbol *templateSymbol) {
    auto    parentScope = templateSymbol->scope;
    auto   templateNode = (TpTemplate*)templateSymbol->node;
    auto   instanceNode = mem.New<TpStruct>(templateNode->pos, templateNode->dotName, TpStruct::OrdinaryStruct);
    auto instanceSymbol = mem.New<TpSymbol>(parentScope, templateSymbol->name, instanceNode);
    instanceNode->modifiers = templateNode->modifiers;
    return exchangeTemplateSymbolWithInstanceSymbol(templateSymbol, instanceSymbol);
}

tp_template_instance_pair tp_mk::Function(TpSymbol *templateSymbol) {
    auto    parentScope = templateSymbol->scope;
    auto   templateNode = (TpTemplate*)templateSymbol->node;
    auto     syntaxNode = (FunctionSyntax*)templateNode->syntax;
    auto   instanceNode = mem.New<TpFunction>(templateNode->pos, syntaxNode->pos.keyword, templateNode->dotName);
    auto instanceSymbol = mem.New<TpSymbol>(parentScope, templateSymbol->name, instanceNode);
    instanceNode->modifiers = templateNode->modifiers;
    return exchangeTemplateSymbolWithInstanceSymbol(templateSymbol, instanceSymbol);
}

tp_template_instance_pair tp_mk::Extern(TpSymbol *templateSymbol, Identifier dllPath) {
    auto    parentScope = templateSymbol->scope;
    auto   templateNode = (TpTemplate*)templateSymbol->node;
    auto     syntaxNode = (FunctionSyntax*)templateNode->syntax;
    auto   instanceNode = mem.New<TpFunction>(templateNode->pos, syntaxNode->pos.keyword, templateNode->dotName);
    auto instanceSymbol = mem.New<TpSymbol>(parentScope, templateSymbol->name, instanceNode);
    instanceNode->modifiers = templateNode->modifiers;
    instanceNode->dllPath = dllPath;
    return exchangeTemplateSymbolWithInstanceSymbol(templateSymbol, instanceSymbol);
}

TpSymbol* tp_mk::OrdinaryFn(Pos pos, Identifier name) {
    name = ids.random(name->text, name->length);
    auto   instanceNode = mem.New<TpFunction>(tp.mkPos(pos), Keyword::Fn, dotName(name));
    auto instanceSymbol = Symbol(name, instanceNode);
    instanceNode->type = instanceSymbol;
    instanceNode->scope = mem.New<TpScope>(tp.current->scope, instanceSymbol);
    instanceNode->isCompilerGenerated = true;
    return instanceSymbol;
}

TpSymbol* tp_mk::NextFn(Pos pos) {
    auto stSymbol = tp.current->scope->owner;
    auto   stNode = (TpStruct*)stSymbol->node;
    auto     name = ids.random(ids.kw_next);
    auto   fnNode = mem.New<TpFunction>(tp.mkPos(pos), Keyword::Fn, dotName(name));
    auto fnSymbol = Symbol(name, fnNode);
    fnNode->type = fnSymbol;
    fnNode->scope = mem.New<TpScope>(tp.current->scope, fnSymbol);
    tp_current current{ fnNode->scope };
    if (tp.enter(current)) {
        auto parameterSymbol = Parameter(pos, ids.kw_this, stNode->type.mkPointer());
        fnNode->parameters.append(parameterSymbol);
        tp.leave(current);
    }
    return fnSymbol;
}

TpSymbol* tp_mk::DisposeFn(Pos pos) {
    auto stSymbol = tp.current->scope->owner;
    auto   stNode = (TpStruct*)stSymbol->node;
    auto     name = ids.random(ids.kw_dispose);
    auto   fnNode = mem.New<TpFunction>(tp.mkPos(pos), Keyword::Fn, dotName(name));
    auto fnSymbol = Symbol(name, fnNode);
    fnNode->type  = fnSymbol;
    fnNode->scope = mem.New<TpScope>(tp.current->scope, fnSymbol);
    fnNode->isCompilerGenerated = true;
    tp_current current{ fnNode->scope };
    if (tp.enter(current)) {
        auto parameterSymbol = Parameter(pos, ids.kw_this, stNode->type.mkPointer());
        fnNode->parameters.append(parameterSymbol);
        fnNode->fnreturn = parameterSymbol->node->type;
        tp.leave(current);
    }
    return fnSymbol;
}

TpSymbol* tp_mk::ResumableStruct(Pos pos) {
    auto           name = ids.random(ids.kw_Resumable);
    auto   instanceNode = mem.New<TpStruct>(tp.mkPos(pos), dotName(name), TpStruct::ResumableStruct);
    auto instanceSymbol = Symbol(name, instanceNode);
    instanceNode->type  = instanceSymbol;
    instanceNode->scope = mem.New<TpScope>(tp.current->scope, instanceSymbol);
    return instanceSymbol;
}

tp_template_instance_pair tp_mk::LambdaStruct(TpSymbol *templateSymbol) {
    auto    parentScope = templateSymbol->scope;
    auto   templateNode = (TpTemplate*)templateSymbol->node;
    auto   instanceNode = mem.New<TpStruct>(templateNode->pos, templateNode->dotName, TpStruct::LambdaStruct);
    auto instanceSymbol = mem.New<TpSymbol>(parentScope, templateSymbol->name, instanceNode);
    return exchangeTemplateSymbolWithInstanceSymbol(templateSymbol, instanceSymbol);
}

TpSymbol* tp_mk::LambdaFunction(Pos pos) {
    auto stSymbol = tp.current->scope->owner;
    auto   stNode = (TpStruct*)stSymbol->node;
    auto     name = ids.kw_open_close_parens;
    auto   fnNode = mem.New<TpFunction>(tp.mkPos(pos), Keyword::Fn, dotName(name));
    auto fnSymbol = Symbol(name, fnNode);
    fnNode->type = fnSymbol;
    fnNode->scope = mem.New<TpScope>(tp.current->scope, fnSymbol);
    tp_current current{ fnNode->scope };
    if (tp.enter(current)) {
        auto parameterSymbol = Parameter(pos, ids.kw_this, stNode->type.mkReference());
        fnNode->parameters.append(parameterSymbol);
        tp.leave(current);
    }
    return fnSymbol;
}

TpSymbol* tp_mk::Symbol(Identifier name, TpSymbolNode *node) {
    return Symbol(tp.current->scope, name, node);
}

TpSymbol* tp_mk::Symbol(TpScope *scope, Identifier name, TpSymbolNode *node) {
    auto symbol = mem.New<TpSymbol>(scope, name, node);
    scope->symbols.append(name, symbol);
    return symbol;
}

TpNode* tp_mk::Defer(Pos pos, TpNode *value) {
    return mem.New<TpDefer>(tp.mkPos(pos), value);
}

TpNode* tp_mk::Return(Pos pos, TpNode *retval) {
    if (auto fnSymbol = tp.current->function()) {
        auto   fnNode = (TpFunction*)fnSymbol->node;
        if (retval == nullptr) {
            if (fnNode->fnreturn.isUnknown() || fnNode->fnreturn.isVoid()) {
                fnNode->fnreturn = tp.tree.tyVoid;
                return mem.New<TpReturn>(tp.mkPos(pos));
            }
            retval = ZeroOf(pos, fnNode->fnreturn);
            return mem.New<TpReturn>(tp.mkPos(pos), retval);
        }
        if (fnNode->fnreturn.isUnknown()) {
            fnNode->fnreturn = retval->type;
            return mem.New<TpReturn>(tp.mkPos(pos), retval);
        }
        if (retval = tp.cast(pos, retval, fnNode->fnreturn, tp_cast_reason::ExplicitCast)) {
            return mem.New<TpReturn>(tp.mkPos(pos), retval);
        }
    } else {
        type_error(retval, "cannot %kw outside of a function", Keyword::Return);
    }
    return tp.throwAway(retval);
}

TpNode* tp_mk::Break(Pos pos) {
    if (tp.current->isaLoop()) {
        return mem.New<TpBreak>(tp.mkPos(pos));
    }
    syntax_error(pos, "%kw expected only in loops", Keyword::Break);
    return nullptr;
}

TpNode* tp_mk::Continue(Pos pos) {
    if (tp.current->isaLoop()) {
        return mem.New<TpBreak>(tp.mkPos(pos));
    }
    syntax_error(pos, "%kw expected only in loops", Keyword::Continue);
    return nullptr;
}

TpBlock* tp_mk::Block(Pos syntax) {
    auto     pos = tp.mkPos(syntax);
    auto    name = ids.random(ids.kw_block);
    auto   label = mem.New<TpLabel>(pos, dotName(name));
    auto  symbol = Symbol(name, label);
    label->scope = mem.New<TpScope>(tp.current->scope, symbol);
    label->block = mem.New<TpBlock>(pos, label);
    label->type  = symbol;
    tp.current->scope->append(label->block);
    return (TpBlock*)label->block;
}

TpIfBlock* tp_mk::IfBlock(Pos syntax) {
    auto     pos = tp.mkPos(syntax);
    auto    name = ids.random(ids.kw_if);
    auto   label = mem.New<TpLabel>(pos, dotName(name));
    auto  symbol = Symbol(name, label);
    label->scope = mem.New<TpScope>(tp.current->scope, symbol);
    label->block = mem.New<TpIfBlock>(pos, label);
    label->type  = symbol;
    tp.current->scope->append(label->block);
    return (TpIfBlock*)label->block;
}

TpLoop* tp_mk::Loop(Pos syntax) {
    auto     pos = tp.mkPos(syntax);
    auto    name = ids.random(ids.kw_loop);
    auto   label = mem.New<TpLabel>(pos, dotName(name));
    auto  symbol = Symbol(name, label);
    label->scope = mem.New<TpScope>(tp.current->scope, symbol);
    label->block = mem.New<TpLoop>(pos, label);
    label->type = symbol;
    tp.current->scope->append(label->block);
    return (TpLoop*)label->block;
}

TpNode* tp_mk::UnaryPrefix(Pos pos, Tok op, TpNode *value) {
    auto result = mem.New<TpUnaryPrefix>(tp.mkPos(pos), op, value);
    if (tp.isa.ConstExpr(value)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::PointerUnaryPrefix(Pos pos, Tok op, TpNode *value) {
    auto result = mem.New<TpPointerUnaryPrefix>(tp.mkPos(pos), op, value);
    if (tp.isa.ConstExpr(value)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::UnarySuffix(Pos pos, TpNode *value, Tok op) {
    auto result = mem.New<TpUnarySuffix>(tp.mkPos(pos), value, op);
    if (tp.isa.ConstExpr(value)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::PointerUnarySuffix(Pos pos, TpNode *value, Tok op) {
    auto result = mem.New<TpPointerUnarySuffix>(tp.mkPos(pos), value, op);
    if (tp.isa.ConstExpr(value)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::SizeOf(Pos pos, TpNode *value) {
    tp.throwAway(value);
    if (value->type.isIndirect() || value->type.isaFunctionOrFunctionTemplate()) {
        return tp.mk.Literal(pos, tp.tree.tyInt32, SIZEOF_POINTER);
    } 
    if (auto sym = value->type.isaBuiltin()) {
        auto node = (TpBuiltin*)sym->node;
        return tp.mk.Literal(pos, tp.tree.tyInt32, node->size);
    }
    if (auto sym = value->type.isanEnum()) {
        auto  en = (TpEnum*)sym->node;
        if (en->valueType.isIndirect() || en->valueType.isaFunctionOrFunctionTemplate()) {
            return tp.mk.Literal(pos, tp.tree.tyInt32, SIZEOF_POINTER);
        }
        if (sym  = en->valueType.isaBuiltin()) {
            auto node = (TpBuiltin*)sym->node;
            return tp.mk.Literal(pos, tp.tree.tyInt32, node->size);
        }
    }
    auto result = mem.New<TpSizeOf>(tp.mkPos(pos), value->type);
    return ConstExpr(pos, result);
}

TpNode* tp_mk::TypeOf(Pos pos, TpNode *value) {
    if (auto tpname = tp.isa.TypeName(value)) {
        return tpname;
    }
    tp.throwAway(value);
    return TypeName(pos, value->type);
}

TpNode* tp_mk::Delete(Pos pos, TpNode *value) {
    return mem.New<TpDelete>(tp.mkPos(pos), value);
}

TpNode* tp_mk::Atomic(Pos pos, TpNode *value) {
    return mem.New<TpAtomic>(tp.mkPos(pos), value);
}

TpDefinition* tp_mk::Definition(Pos pos, TpSymbol *lhs, TpNode *rhs) {
    auto name = mem.New<TpValueName>(lhs->node->pos, lhs);
    return mem.New<TpDefinition>(tp.mkPos(pos), name, rhs);
}

TpAssignment* tp_mk::Assignment(Pos pos, TpNode *dst, TpNode *src) {
    return mem.New<TpAssignment>(tp.mkPos(pos), dst, src);
}

TpAssignment* tp_mk::CompoundPointerArithmetic(Pos pos, TpNode *dst, Tok op, TpNode *src) {
    return mem.New<TpCompoundPointerArithmetic>(tp.mkPos(pos), dst, op, src);
}

TpAssignment* tp_mk::CompoundShift(Pos pos, TpNode *dst, Tok op, TpNode *src) {
    return mem.New<TpCompoundShift>(tp.mkPos(pos), dst, op, src);
}

TpAssignment* tp_mk::CompoundArithmetic(Pos pos, TpNode *dst, Tok op, TpNode *src) {
    return mem.New<TpCompoundArithmetic>(tp.mkPos(pos), dst, op, src);
}

TpNode* tp_mk::PointerPlusInt(Pos pos, TpNode *lhs, TpNode *rhs) {
    auto result = mem.New<TpPointerArithmetic>(tp.mkPos(pos), lhs->type, lhs, Tok::Plus, rhs);
    if (tp.isa.ConstExpr(lhs) && tp.isa.ConstExpr(rhs)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::IntPlusPointer(Pos pos, TpNode *lhs, TpNode *rhs) {
    auto result = mem.New<TpPointerArithmetic>(tp.mkPos(pos), rhs->type, lhs, Tok::Plus, rhs);
    if (tp.isa.ConstExpr(lhs) && tp.isa.ConstExpr(rhs)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::PointerMinusInt(Pos pos, TpNode *lhs, TpNode *rhs) {
    auto result = mem.New<TpPointerArithmetic>(tp.mkPos(pos), lhs->type, lhs, Tok::Minus, rhs);
    if (tp.isa.ConstExpr(lhs) && tp.isa.ConstExpr(rhs)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::IntMinusPointer(Pos pos, TpNode *lhs, TpNode *rhs) {
    auto result = mem.New<TpPointerArithmetic>(tp.mkPos(pos), rhs->type, lhs, Tok::Minus, rhs);
    if (tp.isa.ConstExpr(lhs) && tp.isa.ConstExpr(rhs)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::PointerMinusPointer(Pos pos, TpNode *lhs, TpNode *rhs) {
    auto result = mem.New<TpPointerArithmetic>(tp.mkPos(pos), tp.tree.tyInt64, lhs, Tok::Minus, rhs);
    if (tp.isa.ConstExpr(lhs) && tp.isa.ConstExpr(rhs)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::DereferenceIfReference(Pos pos, TpNode *value) {
    if (value != nullptr) {
        if (auto  tpname = tp.isa.TypeName(value)) {
            if (auto ref = tpname->type.isaReference()) {
                tp.throwAway(tpname);
                return mem.New<TpTypeName>(tp.mkPos(pos), ref->pointee);
            }
        } else if (value->type.isaReference()) {
            if (auto ref = tp.isa.ReferenceOf(value)) {
                auto res = ref->value;
                tp.throwAway(ref);
                return res;
            }
            return mem.New<TpDereference>(tp.mkPos(pos), value);
        }
    }
    return value;
}

TpNode* tp_mk::Dereference(Pos pos, TpNode *value) {
    //    T& → T
    // or T* → T
    if (value->type.isDirect()) {
        return value;
    }
    if (auto ref = tp.isa.ReferenceOf(value)) {
        auto res = ref->value;
        tp.throwAway(ref);
        return res;
    }
    if (auto ptr = tp.isa.AddressOf(value)) {
        auto res = ptr->value;
        tp.throwAway(ptr);
        return res;
    }
    return mem.New<TpDereference>(tp.mkPos(pos), value);
}

TpNode* tp_mk::AddressOf(Pos pos, TpNode *value) {
    // T → T*
    if (auto ref = tp.isa.ReferenceOf(value)) {
        auto res = ref->value;
        tp.throwAway(ref);
        value = res;
    }
    return mem.New<TpAddressOf>(tp.mkPos(pos), value);
}

TpNode* tp_mk::ReferenceOf(Pos pos, TpNode *value) {
    // T → T&
    return mem.New<TpReferenceOf>(tp.mkPos(pos), value);
}

TpNode* tp_mk::Shift(Pos pos, TpNode *lhs, Tok op, TpNode *rhs) {
    auto result = mem.New<TpShift>(tp.mkPos(pos), lhs, op, rhs);
    if (tp.isa.ConstExpr(lhs) && tp.isa.ConstExpr(rhs)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::Arithmetic(Pos pos, TpNode *lhs, Tok op, TpNode *rhs) {
    auto result = mem.New<TpArithmetic>(tp.mkPos(pos), lhs, op, rhs);
    if (tp.isa.ConstExpr(lhs) && tp.isa.ConstExpr(rhs)) {
        return ConstExpr(pos, result);
    }
    return result;
}

TpNode* tp_mk::Condition(Pos pos, TpNode *lhs, Tok op, TpNode *rhs) {
    if (lhs != nullptr && rhs != nullptr) {
        auto result = mem.New<TpCondition>(tp.mkPos(pos), lhs, op, rhs);
        if (tp.isa.ConstExpr(lhs) && tp.isa.ConstExpr(rhs)) {
            return ConstExpr(pos, result);
        }
        return result;
    }
    return nullptr;
}

TpNode* tp_mk::Condition(Pos pos, TpNode *lhs) {
    if (lhs != nullptr) {
        if (tp.isa.Condition(lhs)) {
            return lhs;
        }
        return Condition(pos, lhs, Tok::NotEqual, ZeroOf(pos, lhs->type));
    }
    return nullptr;
}

TpNode* tp_mk::Ternary(Pos pos, TpNode *condition, TpNode *iftrue, TpNode *ifalse) {
    auto result = mem.New<TpTernary>(tp.mkPos(pos), condition, iftrue, ifalse);
    if (tp.isa.ConstExpr(condition) && tp.isa.ConstExpr(iftrue) && tp.isa.ConstExpr(ifalse)) {
        return ConstExpr(pos, result);
    }
    return result;
}

//----------------------------------------------------------
TpNode* tp_mk::ZeroOf(Pos pos, TpTypeName *tpname) {
    auto node = ZeroOf(pos, tpname->type);
    tp.throwAway(tpname);
    return node;
}

TpNode* tp_mk::ZeroOf(Pos pos, Type type) {
    return Literal(pos, type, 0ui64);
}

TpNode* tp_mk::Yield_(Pos pos, TpNode *value) {
    if (auto fnSymbol = tp.current->function()) {
        auto   fnNode = (TpFunction*)fnSymbol->node;
        if (fnNode->isaGeneratorFunction()) {
            if (fnNode->fnreturn.isUnknown()) {
                fnNode->fnreturn = value->type;
                return mem.New<TpYield>(tp.mkPos(pos), value);
            } 
            if (value = tp.cast(pos, value, fnNode->fnreturn, tp_cast_reason::ExplicitCast)) {
                return mem.New<TpYield>(tp.mkPos(pos), value);
            }
        } else {
            type_error(value, "cannot %kw outside of a generator function", Keyword::Yield);
        }
    } else {
        type_error(value, "cannot %kw outside of a generator function", Keyword::Yield);
    }
    return tp.throwAway(value);
}

TpNode* tp_mk::YieldFrom(Pos pos, TpNode *value) {
    if (auto stSymbol = tp.isa.ResumableStruct(value->type)) {
        auto   stNode = (TpStruct*)stSymbol->node;
        if (auto nextfnFieldSymbol = stNode->scope->contains(ids.kw_next)) {
            auto   nextfnFieldNode = (TpField*)nextfnFieldSymbol->node;
            if (auto  nextfnSymbol = nextfnFieldNode->type.isaFunction()) {
                auto    nextfnNode = (TpFunction*)nextfnSymbol->node;
                if (nextfnNode->fnreturn.isUnknown()) {
                    type_error(pos, "are you recursing: %tptype", &nextfnNode->type);
                } else if (auto fnSymbol = tp.current->function()) {
                    auto   fnNode = (TpFunction*)fnSymbol->node;
                    if (fnNode->isaGeneratorFunction()) {
                        if (fnNode->fnreturn.isUnknown()) {
                            fnNode->fnreturn = nextfnNode->fnreturn;
                            return mem.New<TpYieldFrom>(tp.mkPos(pos), value);
                        }
                        if (nextfnNode->fnreturn != fnNode->fnreturn) {
                            type_error(value, "%kw %kw %tptype → %tptype", Keyword::Yield, Keyword::From,
                                       &nextfnNode->fnreturn, &fnNode->fnreturn); 
                        } else {
                            return mem.New<TpYieldFrom>(tp.mkPos(pos), value);
                        }
                    } else {
                        type_error(value, "cannot %kw %kw outside of a generator function", Keyword::Yield, Keyword::From);
                    }
                } else {
                    type_error(value, "cannot %kw %kw outside of a generator function", Keyword::Yield, Keyword::From);
                }
            } else {
                type_error(pos, "%s#<red> is not a function: %tptype", ids.kw_next, 
                           &nextfnFieldNode->type);
            }
        } else {
            type_error(pos, "%s#<red> not found in %tptype", ids.kw_next, &stNode->type);
        }
    } else {
        type_error(value, "cannot %kw %kw a %tptype", Keyword::Yield, Keyword::From, &value->type);
    }
    return tp.throwAway(value);
}

TpNode* tp_mk::Await(Pos pos, TpNode *value) {
    if (auto fnSymbol = tp.current->function()) {
        auto   fnNode = (TpFunction*)fnSymbol->node;
        if (fnNode->isaGeneratorFunction()) {
            return mem.New<TpAwait>(tp.mkPos(pos), fnNode->fnreturn, value);
        }
    }
    type_error(value, "cannot %kw outside of a generator function", Keyword::Await);
    return tp.throwAway(value);
}

TpCall* tp_mk::FunctionCallFromSite(TpNode *name, TpSymbol *fnSymbol) {
    auto    fnNode = (TpFunction*)fnSymbol->node;
    auto &fnreturn = fnNode->fnreturn;
    auto     &site = *tp.current->site;
    auto      call = mem.New<TpCall>(tp.mkPos(site.pos), fnreturn, name);
    for (auto i = 0; i < site.arguments.list.length; i++) {
        auto &argument = site.arguments.list.items[i];
        call->arguments.append(argument.value);
        argument.value = nullptr;
    }
    Assert(fnNode->parameters.length == site.arguments.list.length);
    return call;
}

TpInitializer* tp_mk::StructInitializerFromSite(Type type) {
    auto &site = *tp.current->site;
    auto  init = mem.New<TpInitializer>(tp.mkPos(site.pos), type);
    for (auto i = 0; i < site.arguments.list.length; i++) {
        Assert(0);
        auto &argument = site.arguments.list.items[i];
        init->arguments.append(argument.value);
        argument.value = nullptr;
    }
    return init;
}

//----------------------------------------------------------
TpNode* tp_mk::Name(Pos pos, TpSymbol *symbol) {
    if (symbol == nullptr) {
        return nullptr;
    }
    if (tp.isa.TypeAliasSymbol(symbol)) {
        return TypeName(pos, symbol->node->type);
    }
    if (auto alias = tp.isa.ValueAlias(symbol)) {
        return ValueName(pos, alias->value);
    }
    if (auto alias = tp.isa.ConstAlias(symbol)) {
        if (auto expr = tp.isa.ConstExpr(alias->value)) {
            return ConstExpr(pos, expr->value);
        }
        return ConstExpr(pos, alias->value);
    }
    if (tp.isa.TypeSymbol(symbol)) {
        return TypeName(pos, symbol->node->type);
    }
    if (tp.isa.ValueSymbol(symbol)) {
        return ValueName(pos, symbol);
    }
    return nullptr;
}

TpNode* tp_mk::TypeName(Pos pos, Type type) {
    if (auto moduleSymbol = type.isaModule()) {
        tp.bindModule(moduleSymbol);
    } else if (auto templateSymbol = type.isaTemplate()) {
        if (auto    instanceSymbol = instantiateIfZeroArity(templateSymbol)) {
            return mem.New<TpTypeName>(tp.mkPos(pos), instanceSymbol);
        }
        return nullptr;
    }
    return mem.New<TpTypeName>(tp.mkPos(pos), type);
}

TpNode* tp_mk::ValueName(Pos pos, TpSymbol *symbol) {
    Assert(tp.isa.ValueSymbol(symbol));
    if (symbol->node->kind == TpKind::Field) {
        type_error(pos, "cannot access field %s#<red> without an instance", symbol->name);
        return nullptr;
    }
    return mem.New<TpValueName>(tp.mkPos(pos), symbol);
}

TpNode* tp_mk::ConstExpr(Pos pos, TpNode *value) {
    if (auto expr = tp.isa.ConstExpr(value)) {
        return mem.New<TpConstExpr>(tp.mkPos(pos), expr->value);
    }
    return mem.New<TpConstExpr>(tp.mkPos(pos), value);
}

TpNode* tp_mk::FieldName(Pos pos, TpNode *base, TpSymbol *field) {
    if (base->type.isIndirect()) {
        base = Dereference(pos, base);
    }
    if (base != nullptr) {
        auto name = mem.New<TpFieldName>(tp.mkPos(pos), base, field);
        if (tp.isa.PassByReference(name->type)) {
            return ReferenceOf(pos, name);
        }
        return name;
    }
    return nullptr;
}

TpNode* tp_mk::IndexName(Pos pos, TpNode *base, TpNode *index) {
    if (index->type.isSigned()) {
        index = tp.cast(pos, index, tp.tree.tyInt64, tp_cast_reason::ImplicitCast);
    } else if (index->type.isUnsigned()) {
        index = tp.cast(pos, index, tp.tree.tyUInt64, tp_cast_reason::ImplicitCast);
    } else {
        type_error(pos, "cannot be an indexer: %tptype", &index->type);
        return tp.throwAway(base, index);
    }
    if (index == nullptr) {
        return tp.throwAway(base);
    }
    if (auto ptr = base->type.isaPointer()) {
        auto  &type = ptr->pointee;
        auto result = mem.New<TpIndexName>(tp.mkPos(pos), type, base, index);
        if (tp.isa.PassByReference(type)) {
            return ReferenceOf(pos, result);
        }
        return result;
    }
    Assert(0);
    return tp.throwAway(base, index);
}

TpNode* tp_mk::Name(Pos pos, TpNode *base, TpSymbol *symbol) {
    if (tp.isa.Field(symbol)) {
        if (!tp.isa.TypeName(base)) {
            if (base->type.isIndirect()) {
                base = Dereference(pos, base);
                if (base == nullptr) {
                    return nullptr;
                }
                if (base->type.isIndirect()) {
                    type_error(pos, "cannot access field %s#<red> from %tptype", symbol->name, 
                               &base->type);
                    return (TpName*)tp.throwAway(base);
                }
            }
            auto   baseSymbol = base->type.isDirect();
            if (auto baseNode = tp.isa.TypeNode(baseSymbol->node)) {
                if (baseNode->scope == symbol->scope) {
                    return FieldName(pos, base, symbol);
                }
            }
        }
        type_error(pos, "cannot access field %s#<red> except from an instance of %tptype",
                   symbol->name, &base->type);
        return (TpName*)tp.throwAway(base);
    }
    tp.throwAway(base);
    return Name(pos, symbol);
}

//----------------------------------------------------------
TpConstExpr* tp_mk::Literal(Pos syntax, Type type, UINT64 u64) {
    auto   pos = tp.mkPos(syntax);
    auto value = mem.New<TpLiteral>(pos, type, u64);
    return mem.New<TpConstExpr>(pos, value);
}

//----------------------------------------------------------
Identifier tp_mk::dotName(Identifier rhs) {
    return dotName(tp.current->scope, rhs);
}

Identifier tp_mk::dotName(TpScope *scope, Identifier rhs) {
    if (scope->owner == nullptr) {
        return rhs;
    }
    return dotName(scope->owner, rhs);
}

Identifier tp_mk::dotName(TpSymbol *lhs, Identifier rhs) {
    return dotName(lhs->node, rhs);
}

Identifier tp_mk::dotName(TpSymbolNode *lhs, Identifier rhs) {
    return dotName(lhs->dotName, rhs);
}

Identifier tp_mk::dotName(Identifier lhs, Identifier rhs) {
    String s{};
    s.append(lhs).append(".").append(rhs);
    auto id = ids.get(s);
    s.dispose();
    return id;
}

TpSymbol* tp_mk::instantiateIfZeroArity(TpSymbol * templateSymbol) {
    auto templateNode = (TpTemplate*)templateSymbol->node;
    if (templateNode->arity.isNotZero()) {
        return templateSymbol;
    }
    if (templateNode->instances.isNotEmpty()) {
        return templateNode->instances.first();
    }
    auto syntaxNode = templateNode->syntax;
    tp_site site{ syntaxNode };
    auto instanceSymbol = site.bindTemplate(templateSymbol);
    site.dispose();
    return instanceSymbol;
}
} // namespace exy