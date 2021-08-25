#include "pch.h"
#include "typer.h"

#define site_error(pos, msg, ...) diagnostic("Site", pos, msg, __VA_ARGS__)

namespace exy {
tp_argument::tp_argument(SyntaxNode *syntax) : syntax(syntax) {}

void tp_argument::dispose() {
    value = typer->throwAway(value);
}

//----------------------------------------------------------
void tp_argument_list::dispose() {
    list.dispose([](auto &x) { x.dispose(); });
}

bool tp_argument_list::set(TpNode *receiver, EnclosedSyntax *arguments, FunctionSyntax *with) {
    const auto errors = compiler.errors;
    auto          &tp = *typer;
    syntax = arguments;
    if (receiver != nullptr) {
        auto &argument = list.place(syntax);
        argument.name = ids.kw_this;
        argument.value = receiver;
    }
    if (syntax == nullptr) {
        Assert(0);
    } else if (syntax->value == nullptr) {
        // Do nothing.
    } else if (auto commaSeparated = tp.isa.CommaSeparatedSyntax(syntax->value)) {
        for (auto i = 0; i < commaSeparated->nodes.length; i++) {
            list.place(commaSeparated->nodes.items[i]);
        }
    } else {
        list.place(syntax->value);
    }
    auto named = 0;
    for (auto i = 0; i < list.length; i++) {
        auto &argument = list.items[i];
        if (argument.isReceiver()) {
            continue;
        }
        if (auto nv = tp.isa.NameValueSyntax(argument.syntax)) {
            argument.name = nv->name->value;
            argument.value = tp.bindExpression(nv->value);
            ++named;
            if (argument.name == ids.kw_this) {
                syntax_error(nv->name, "cannot use %s#<red> as an argument name", argument.name);
            }
        } else {
            argument.value = tp.bindExpression(argument.syntax);
            if (named > 0) {
                syntax_error(argument.syntax, "un-named argument after named argument(s)");
            }
        }
    }
    if (errors == compiler.errors && with != nullptr) {
        if (auto value = tp.bindExpression(with)) {
            auto &argument = list.place(with);
            argument.value = value;
        }
    }
    if (errors == compiler.errors && tp.isa.ParenthesizedSyntax(syntax)) {
        for (auto i = 0; i < list.length; i++) {
            auto &argument = list.items[i];
            if (tp.isa.PassByReference(argument.value->type)) {
                argument.value = tp.mk.ReferenceOf(argument.syntax, argument.value);
            }
        }
    }
    return errors == compiler.errors;
}

tp_argument& tp_argument_list::append(SyntaxNode *pos, TpNode *value) {
    auto       &tp = *typer;
    auto &argument = list.place(pos);
    if (tp.isa.PassByReference(value->type)) {
        value = tp.mk.ReferenceOf(pos, value);
    }
    argument.value = value;
    return argument;
}

INT tp_argument_list::indexOf(Identifier name) {
    for (auto i = 0; i < list.length; i++) {
        const auto &argument = list.items[i];
        if (argument.name == name) {
            return i;
        }
    }
    return -1;
}

//----------------------------------------------------------
tp_parameter::tp_parameter(SyntaxNode *syntax) 
    : syntax(syntax) {}

void tp_parameter::dispose() {
    auto &tp = *typer;
    defaultValue = tp.throwAway(defaultValue);
}

bool tp_parameter::isaRequiredParameter() const {
    return !isaDefaultParameter() && !isaRestParameter();
}

bool tp_parameter::isaDefaultParameter() const {
    return defaultValue != nullptr;
}

bool tp_parameter::isaRestParameter() const {
    auto &tp = *typer;
    return tp.isa.RestParameterSyntax(syntax) != nullptr;
}

//----------------------------------------------------------
void tp_parameter_list::dispose() {
    list.dispose([](auto &x) { x.dispose(); });
}

bool tp_parameter_list::set(TpScope *scope, ParenthesizedSyntax *parameters) {
    const auto errors = compiler.errors;
    auto &tp = *typer;
    syntax = parameters;
    auto prevScope = tp.current->scope;
    tp_current current{ scope };
    if (tp.enter(current)) {
        if (syntax == nullptr || syntax->value == nullptr) {
            // Do nothing.
        } else if (auto commaSeparated = tp.isa.CommaSeparatedSyntax(syntax->value)) {
            for (auto i = 0; i < commaSeparated->nodes.length; i++) {
                list.place(commaSeparated->nodes.items[i]);
            }
        } else {
            list.place(syntax->value);
        }
        for (auto i = 0; i < list.length; i++) {
            auto &parameter = list.items[i];
            if (auto var = tp.isa.VariableSyntax(parameter.syntax)) {
                parameter.modifiers = var->modifiers;
                if (auto id = tp.isa.IdentifierSyntax(var->name)) {
                    parameter.name = id->value;
                } else if (auto nv = tp.isa.NameValueSyntax(var->name)) {
                    Assert(nv->op.kind == Tok::Colon);
                    parameter.name = nv->name->value;
                    if (auto expr = tp.bindExpression(nv->value)) {
                        if (auto tpname = tp.isa.TypeName(expr)) {
                            parameter.type = tpname->type;
                        } else {
                            site_error(nv->value, "expected a typename, not a value of %tptype",
                                       &expr->type);
                        }
                        tp.throwAway(expr);
                    }
                } else {
                    syntax_error(var->name, "expected an identifier, not %sk", var->name->kind);
                }
                if (auto id = tp.isa.IdentifierSyntax(var->rhs)) {
                    auto kw = id->pos.keyword;
                    if (kw > Keyword::_begin_compiler_keywords && kw < Keyword::_end_compiler_keywords) {
                        tp_current cur{ prevScope };
                        if (tp.enter(cur)) {
                            parameter.defaultValue = tp.bindExpression(id);
                            tp.leave(cur);
                        }
                    } else {
                        parameter.defaultValue = tp.bindExpression(id);
                    }
                } else {
                    parameter.defaultValue = tp.bindExpression(var->rhs);
                }
                if (parameter.defaultValue != nullptr) {
                    if (parameter.type.isKnown()) {
                        parameter.defaultValue = tp.cast(var->rhs, parameter.defaultValue,
                                                         parameter.type, tp_cast_reason::ExplicitCast);
                    }
                }
            } else if (auto rest = tp.isa.RestParameterSyntax(parameter.syntax)) {
                parameter.modifiers = rest->modifiers;
                if (auto id = rest->name) {
                    parameter.name = id->value;
                } else {
                    parameter.name = ids.kw__args__;
                }
            } else {
                syntax_error(parameter.syntax, "expected a variable or '...' parameter");
            }
        }
        if (errors == compiler.errors && tp.isa.ParenthesizedSyntax(syntax)) {
            for (auto i = 0; i < list.length; i++) {
                auto &parameter = list.items[i];
                if (parameter.type.isKnown()) {
                    if (tp.isa.PassByReference(parameter.type)) {
                        type_error(parameter.syntax, "expected a pass-by-value parameter type, not %tptype",
                                   &parameter.type);
                    }
                } else if (parameter.defaultValue != nullptr) {
                    if (tp.isa.PassByReference(parameter.defaultValue->type)) {
                        parameter.defaultValue = tp.mk.ReferenceOf(parameter.syntax, parameter.defaultValue);
                    }
                }
            }
        }
        tp.leave(current);
    }
    return errors == compiler.errors;
}

bool tp_parameter_list::set(TpScope *scope, AngledSyntax *parameters) {
    const auto errors = compiler.errors;
    auto &tp = *typer;
    syntax = parameters;
    tp_current current{ scope };
    if (tp.enter(current)) {
        if (syntax == nullptr || syntax->value == nullptr) {
            // Do nothing.
        } else if (auto commaSeparated = tp.isa.CommaSeparatedSyntax(syntax->value)) {
            for (auto i = 0; i < commaSeparated->nodes.length; i++) {
                list.place(commaSeparated->nodes.items[i]);
            }
        } else {
            list.place(syntax->value);
        }
        for (auto i = 0; i < list.length; i++) {
            auto &parameter = list.items[i];
            if (auto id = tp.isa.IdentifierSyntax(parameter.syntax)) {
                parameter.name = id->value;
            } else if (auto nv = tp.isa.NameValueSyntax(parameter.syntax)) {
                parameter.name = nv->name->value;
                Assert(nv->op.kind == Tok::Assign);
                if (auto expr = tp.bindExpression(nv->value)) {
                    if (tp.isa.TypeName(expr) || tp.isa.ConstExpr(expr)) {
                        parameter.type = expr->type;
                        parameter.defaultValue = expr;
                    } else {
                        site_error(nv->value, "expected a typename or constant, not a value of %tptype",
                                   &expr->type);
                        tp.throwAway(expr);
                    }
                }
            } else if (auto rest = tp.isa.RestParameterSyntax(parameter.syntax)) {
                parameter.modifiers = rest->modifiers;
                if (id = rest->name) {
                    parameter.name = id->value;
                } else {
                    parameter.name = ids.kw__args__;
                }
            } else {
                syntax_error(parameter.syntax, "expected an identifier, name-value or '...' parameter");
            }
        }
        tp.leave(current);
    }
    return errors == compiler.errors;
}

bool tp_parameter_list::set(tp_argument_list &arguments) {
    const auto   errors = compiler.errors;
    auto    &parameters = *this;
    auto  argumentIndex = 0;
    auto parameterIndex = 0;
    auto            &tp = *typer;
    // (1) Match each argument to a parameter by name.
    for (argumentIndex = 0; argumentIndex < arguments.list.length; argumentIndex++) {
        auto &argument = arguments.list.items[argumentIndex];
        if (auto argumentName = argument.name) {
            parameterIndex = parameters.indexOf(argumentName);
            if (parameterIndex >= 0) {
                auto &parameter = parameters.list.items[parameterIndex];
                if (parameter.isaRestParameter()) {
                    site_error(argument.syntax, "cannot name a vararg parameter: %s#<red>", argumentName);
                } else if (parameter.isReceiver()) {
                    if (argument.isNotReceiver()) {
                        site_error(argument.syntax, "cannot name a %s#<red> parameter", argumentName);
                    }
                } else if (parameter.argumentIndex >= 0) {
                    site_error(argument.syntax, "%s#<red> already assigned", argumentName);
                } else {
                    parameter.argumentIndex = argumentIndex;
                    argument.parameterIndex = parameterIndex;
                }
            } else {
                site_error(argument.syntax, "%s#<red> is not a parameter", argumentName);
            }
        }
    }
    if (errors != compiler.errors) {
        return false;
    }
    // (2) Except for vararg, assign each argument to a parameter.
    auto restParameterIndex = -1;
    argumentIndex = 0;
    for (parameterIndex = 0; parameterIndex < parameters.list.length; parameterIndex++) {
        auto &parameter = parameters.list.items[parameterIndex];
        if (parameter.argumentIndex >= 0) {
            Assert(parameter.isNotARestParameter());
            continue; // Already assigned.
        }
        if (parameter.isaRestParameter()) {
            restParameterIndex = parameterIndex;
            break;
        }
        for (; argumentIndex < arguments.list.length; ++argumentIndex) {
            auto &argument = arguments.list.items[argumentIndex];
            if (argument.parameterIndex >= 0) {
                continue; // Already assigned.
            }
            argument.name = parameter.name;
            argument.parameterIndex = parameterIndex;
            parameter.argumentIndex = argumentIndex;
            break;
        }
    }
    // (3) Except for vararg, check that each parameter has been assigned an argument.
    for (parameterIndex = 0; parameterIndex < parameters.list.length; parameterIndex++) {
        auto &parameter = parameters.list.items[parameterIndex];
        if (parameterIndex == restParameterIndex) {
            break;
        }
        if (parameter.isaDefaultParameter()) {
            if (parameter.argumentIndex < 0) {
                auto &argument = arguments.list.place(arguments.syntax);
                argument.name = parameter.name;
                argument.value = parameter.defaultValue;
                argument.parameterIndex = parameterIndex;
                parameter.argumentIndex = arguments.list.length - 1;
                parameter.defaultValue = nullptr; // Reset so that it is not disposed.
            }
        } else if (parameter.argumentIndex < 0) {
            site_error(arguments.syntax, "required parameter %s#<red> missing", parameter.name);
        }
    }
    if (errors != compiler.errors) {
        return false;
    }
    // (4) Now assign vararg.
    if (restParameterIndex >= 0) {
        auto restParameter = parameters.list.pop();
        for (argumentIndex = 0; argumentIndex < arguments.list.length; argumentIndex++) {
            auto &argument = arguments.list.items[argumentIndex];
            if (argument.parameterIndex >= 0) {
                continue; // Already assigned a 'required' or 'default' parameter.
            }
            auto &parameter = parameters.list.append(restParameter);
            parameter.argumentIndex = argumentIndex;
            argument.parameterIndex = parameters.list.length - 1;
            argument.name = parameter.name;
        }
    }
    // (5) Ensure that we have as many arguments as parameters and vice versa.
    if (arguments.list.length < parameters.list.length) {
        site_error(arguments.syntax, "too few arguments: expected % i#<green>, found % i# < red>",
                   parameters.list.length, arguments.list.length);
        return false;
    }
    if (arguments.list.length > parameters.list.length) {
        site_error(arguments.syntax, "too many arguments: expected %i#<green>, found %i#<red>",
                   parameters.list.length, arguments.list.length);
        return false;
    }
    if (arguments.list.isNotEmpty()) {
        // (6) Reorder arguments to align with parameters.
        List<tp_argument> newList{};
        newList.reserve(parameters.list.length);
        newList.length = parameters.list.length;
        for (argumentIndex = 0; argumentIndex < arguments.list.length; ++argumentIndex) {
            auto &argument = arguments.list.items[argumentIndex];
            Assert(argument.parameterIndex >= 0 && argument.name != nullptr && argument.value != nullptr);
            Assert(newList.items[argument.parameterIndex].syntax == nullptr);
            newList.items[argument.parameterIndex] = argument;
        }
        arguments.list.dispose();
        arguments.list = newList;
        // (6) Check argument types against typed parameters.
        for (auto i = 0; i < arguments.list.length; ++i) {
            auto  &argument = arguments.list.items[i];
            auto &parameter = parameters.list.items[i];
            if (parameter.type.isUnknown()) {
                parameter.type = argument.value->type;
            } else {
                auto c = tp.canCast(argument.value, parameter.type, tp_cast_reason::ImplicitCast);
                if (c.ok) {
                    argument.value = tp.cast(argument.syntax, argument.value, parameter.type, c);
                } else {
                    c.dispose();
                }
                /*auto upper = tp.upperBound(argument.value->type, parameter.type);
                if (upper.isUnknown()) {
                    site_error(argument.syntax, "incompatible types argument:%tptype vs. parameter:%tptype",
                               &argument.value->type, &parameter.type);
                } else if (argument.value = tp.cast(argument.syntax, argument.value, upper, tp_cast_reason::ImplicitCast)) {
                    argument.value = tp.cast(argument.syntax, argument.value, parameter.type, tp_cast_reason::ImplicitCast);
                }if (upper.isUnknown() || upper != parameter.type) {
                    site_error(argument.syntax, "incompatible types argument:%tptype vs. parameter:%tptype",
                               &argument.value->type, &parameter.type);
                } else {
                    argument.value = tp.cast(argument.syntax, argument.value, parameter.type,
                                             tp_cast_reason::ImplicitCast);
                }*/
            }
        }
    }
    return errors == compiler.errors;
}

bool tp_parameter_list::prependThis(Type type) {
    if (indexOf(ids.kw_this) >= 0) {
        site_error(syntax, "parameter %s#<red> already set", ids.kw_this);
        return false;
    }
    auto &tp = *typer;
    if (tp.isa.PassByReference(type)) {
        site_error(syntax, "expected parameter %s#<red> to be pass-by-reference, found %tptype",
                   ids.kw_this, &type);
        return false;
    }
    auto &parameter = list.insert(0);
    parameter.syntax = syntax;
    parameter.name = ids.kw_this;
    parameter.type = type;
    return true;
}

INT tp_parameter_list::indexOf(Identifier name) {
    for (auto i = 0; i < list.length; i++) {
        const auto &parameter = list.items[i];
        if (parameter.name == name) {
            return i;
        }
    }
    return -1;
}

//----------------------------------------------------------
tp_site::tp_site(SyntaxNode *syntax)
    : tp(*typer), prev(typer->current->site), pos(syntax) {
    tp.current->site = this;
}

void tp_site::dispose() {
    arguments.dispose();
    parameters.dispose();
    tp.current->site = prev;
}

void tp_site::synchronize(TpNode *node) {
    const auto &srwType = tp.sym_aio_SRWLOCK->node->type;
    tp_lookup lookup{};
    if (auto ptr = node->type.isIndirect()) {
        if (ptr->pointee == srwType) {
            // node: aio.SRWLOCK&/*
            auto acquire = lookup.find(pos, node, ids.kw_acquire);
            auto release = lookup.find(pos, node, ids.kw_release);
            if (acquire == nullptr || release == nullptr) {
                tp.throwAway(acquire, release);
            } else {
                // node.acquire()
                lock(node, acquire);
                // defer node.release()
                unlock(node, release);
            }
        } else if (ptr->pointee.isaStruct()) {
            // node: S*/& where S contains a field named 'srw'
            auto srwField = lookup.find(pos, node, ids.kw_srw);
            if (srwField != nullptr) {
                auto acquire = lookup.find(pos, srwField, ids.kw_acquire);
                auto release = lookup.find(pos, srwField, ids.kw_release);
                if (acquire == nullptr || release == nullptr) {
                    tp.throwAway(acquire, release);
                } else {
                    // node.srw.acquire()
                    lock(node, acquire);
                    // defer node.srw.release()
                    unlock(node, release);
                }
            }
        } else {
            site_error(pos, "unsynchronizable: %tptype", &node->type);
        }
    } else if (node->type == srwType) {
        // node: aio.SRWLOCK
        auto acquire = lookup.find(pos, node, ids.kw_acquire);
        auto release = lookup.find(pos, node, ids.kw_release);
        if (acquire == nullptr || release == nullptr) {
            tp.throwAway(acquire, release);
        } else {
            // node.acquire()
            lock(node, acquire);
            // defer node.release()
            unlock(node, release);
        }
    } else if (node->type.isaStruct()) {
        // node: S where S contains a field named 'srw'
        auto srwField = lookup.find(pos, node, ids.kw_srw);
        if (srwField != nullptr) {
            auto acquire = lookup.find(pos, srwField, ids.kw_acquire);
            auto release = lookup.find(pos, srwField, ids.kw_release);
            if (acquire == nullptr || release == nullptr) {
                tp.throwAway(acquire, release);
            } else {
                // node.srw.acquire()
                lock(node, acquire);
                // defer node.srw.release()
                unlock(node, release);
            }
        }
    } else {
        site_error(pos, "unsynchronizable: %tptype", &node->type);
        tp.throwAway(node);
    }
    lookup.dispose();
}

void tp_site::lock(TpNode *receiver, TpNode *fnName) {
    // aio.SRWLOCK.acquire( receiver )
    if (auto tpname = tp.isa.TypeName(fnName)) {
        if (auto templateSymbol = tpname->type.isaFunctionTemplate()) {
            TpNode      *result = nullptr;
            auto   templateNode = (TpTemplate*)templateSymbol->node;
            auto       fnSyntax = (FunctionSyntax*)templateNode->syntax;
            arguments.append(pos, receiver);
            if (parameters.set(templateSymbol->scope, fnSyntax->parameters)) {
                if (parameters.set(arguments)) {
                    if (auto selected = selectInstance(templateSymbol)) {
                        result = tp.mk.FunctionCallFromSite(fnName, selected);
                    } else if (selected = bindTemplate(templateSymbol)) {
                        result = tp.mk.FunctionCallFromSite(fnName, selected);
                    }
                }
            }
            arguments.dispose();
            parameters.dispose();
            tp.current->scope->append(result);
        } else {
            tp.throwAway(receiver, fnName);
        }
        return;
    }
    site_error(pos, "expected %s#<green> function, but found %tptype", ids.kw_acquire, &fnName->type);
    tp.throwAway(receiver, fnName);
}

void tp_site::unlock(TpNode *receiver, TpNode *fnName) {
    // aio.SRWLOCK.release( receiver )
    if (auto tpname = tp.isa.TypeName(fnName)) {
        if (auto templateSymbol = tpname->type.isaFunctionTemplate()) {
            TpNode      *result = nullptr;
            auto   templateNode = (TpTemplate*)templateSymbol->node;
            auto       fnSyntax = (FunctionSyntax*)templateNode->syntax;
            arguments.append(pos, receiver);
            if (parameters.set(templateSymbol->scope, fnSyntax->parameters)) {
                if (parameters.set(arguments)) {
                    if (auto selected = selectInstance(templateSymbol)) {
                        result = tp.mk.FunctionCallFromSite(fnName, selected);
                    } else if (selected = bindTemplate(templateSymbol)) {
                        result = tp.mk.FunctionCallFromSite(fnName, selected);
                    }
                }
            }
            arguments.dispose();
            parameters.dispose();
            if (result != nullptr) {
                tp.current->scope->append(tp.mk.Defer(pos, result));
            }
        } else {
            tp.throwAway(fnName);
        }
        return;
    }
    site_error(pos, "expected %s#<green> function, but found %tptype", ids.kw_release, &fnName->type);
    tp.throwAway(receiver, fnName);
}

TpNode* tp_site::callDispose(TpNode *receiver, TpNode *with) {
    tp_lookup lookup{};
    TpNode  *result = nullptr;
    if (auto fnName = lookup.find(pos, receiver, ids.kw_dispose)) {
        if (auto tpname = tp.isa.TypeName(fnName)) {
            if (auto  overloadSymbol = tpname->type.isOverloadSet()) {
                result = callDisposeOverload(receiver, with, fnName, overloadSymbol);
            } else if (auto templateSymbol = tpname->type.isaFunctionTemplate()) {
                result = callDisposeFunctionTemplate(receiver, with, fnName, templateSymbol);
            } else {
                site_error(pos, "could not find a %s#<yellow> function overload or template in %tptype",
                           ids.kw_dispose, &receiver->type);
                tp.throwAway(receiver, with, fnName);
            }
        } else {
            site_error(pos, "expected a typename, instead found %tpk: %tptype",
                       fnName->kind, &receiver->type);
            tp.throwAway(receiver, with, fnName);
        }
    } else {
        tp.throwAway(receiver, with);
    }
    lookup.dispose();
    return result;
}

TpNode* tp_site::callDisposeOverload(TpNode *receiver, TpNode *with, TpNode *fnName, 
                                     TpSymbol *overloadSymbol) {
    auto overloadSet = (TpOverloadSet*)overloadSymbol->node;
    for (auto i = 0; i < overloadSet->list.length; i++) {
        auto templateSymbol = overloadSet->list.items[i];
        auto   templateNode = (TpTemplate*)templateSymbol->node;
        const auto   &arity = templateNode->arity;
        auto           args = 1; // {this} = {receiver}
        Assert(arity.hasThis);
        if (with != nullptr) {
            ++args;
        }
        if (args == arity.required) {
            return callDisposeFunctionTemplate(receiver, with, fnName, templateSymbol);
        }
    }
    site_error(pos, "no template of %tptype matches the arguments", &overloadSet->type);
    return tp.throwAway(receiver, with);
}

TpNode* tp_site::callDisposeFunctionTemplate(TpNode *receiver, TpNode *with, TpNode *fnName,
                                             TpSymbol *templateSymbol) {
    arguments.append(pos, receiver);
    if (with != nullptr) {
        arguments.append(pos, with);
    }
    if (auto fnTemplate = tp.isa.FunctionTemplate(templateSymbol)) {
        auto   fnSyntax = (FunctionSyntax*)fnTemplate->syntax;
        if (parameters.set(templateSymbol->scope, fnSyntax->parameters)) {
            if (parameters.set(arguments)) {
                if (auto selected = selectInstance(templateSymbol)) {
                    return tp.mk.FunctionCallFromSite(fnName, selected);
                } else if (selected = bindTemplate(templateSymbol)) {
                    return tp.mk.FunctionCallFromSite(fnName, selected);
                }
            }
        }
    }
    site_error(pos, "expected %s#<green> function, but found %tptype", ids.kw_dispose, 
               &templateSymbol->node->type);
    return tp.throwAway(fnName);
}

TpNode* tp_site::tryCallOperator(TpNode *, Tok , TpNode *) {
    return nullptr;
}

TpNode* tp_site::tryCallOperator(Tok, TpNode *) {
    return nullptr;
}

TpNode* tp_site::tryCallIndexer(TpNode *) {
    return nullptr;
}

TpSymbol* tp_site::createAsyncMain(FunctionSyntax *syntax, TpSymbol *asyncSymbol) {
    /*  translation:
    *       fn `main` {
    *           auto `Resumable` = main()
    *           `block` {
    *               `loop` if true {
    *                   auto (_, done) = `Resumable`.next()
    *                   if done {
    *                       break
    *                   }
    *               }
    *           }
    *       }
    */
    auto           name = asyncSymbol->name;
    auto instanceSymbol = tp.mk.OrdinaryFn(syntax, name);
    auto   instanceNode = (TpFunction*)instanceSymbol->node;
    if (instanceSymbol->bindStatus.begin()) {
        traceln("%depth binding %tptype", &instanceNode->type);
        tp_current current{ instanceNode->scope };
        if (tp.enter(current)) {
            instanceNode->modifiers.isStatic = true;
            //---
            instanceNode->fnreturn = tp.tree.tyVoid;
            tp.leave(current);
        }
        instanceSymbol->bindStatus.finish();
        traceln("%depth bound   %tptype", &instanceNode->type);
    }
    return instanceSymbol;
}
TpSymbol* tp_site::selectInstance(TpSymbol *templateSymbol) {
    auto templateNode = (TpTemplate*)templateSymbol->node;
    auto   syntaxNode = templateNode->syntax;
    if (templateNode->instances.isNotEmpty()) {
        for (auto i = 0; i < templateNode->instances.length; i++) {
            auto instanceSymbol = templateNode->instances.items[i];
            switch (syntaxNode->pos.keyword) {
            #define ZM(zName, zText) case Keyword::zName: instanceSymbol = select##zName##Instance(instanceSymbol); break;
                DeclareStructureTypeKeywords(ZM)
            #undef ZM
            #define ZM(zName, zText) case Keyword::zName: instanceSymbol = select##zName##Instance(instanceSymbol); break;
                DeclareFunctionTypeKeywords(ZM)
            #undef ZM
            }
            if (instanceSymbol != nullptr) {
                return instanceSymbol;
            }
        }
    }
    return nullptr;
}

TpSymbol* tp_site::bindTemplate(TpSymbol *templateSymbol) {
    auto templateNode = (TpTemplate*)templateSymbol->node;
    auto   syntaxNode = templateNode->syntax;
    switch (syntaxNode->pos.keyword) {
    #define ZM(zName, zText) case Keyword::zName: return bind##zName##Template(templateSymbol);
        DeclareStructureTypeKeywords(ZM)
    #undef ZM
    #define ZM(zName, zText) case Keyword::zName: return bind##zName##Template(templateSymbol);
        DeclareFunctionTypeKeywords(ZM)
    #undef ZM
    }
    impl_error(templateSymbol, "bindTemplate");
    return nullptr;
}

TpSymbol* tp_site::bindStructTemplate(TpSymbol *symbol) {
    const auto   errors = compiler.errors;
    auto           pair = tp.mk.Struct(symbol);
    auto templateSymbol = pair.templateSymbol;
    auto instanceSymbol = pair.instanceSymbol;
    auto   instanceNode = (TpStruct*)instanceSymbol->node;
    auto   templateNode = (TpTemplate*)templateSymbol->node;
    auto     syntaxNode = (StructureSyntax*)templateNode->syntax;
    Assert(templateSymbol->bindStatus.isIdle());
    Assert(instanceSymbol->bindStatus.isIdle());
    if (instanceSymbol->bindStatus.begin()) {
        traceln("%depth binding %tptype", &instanceNode->type);
        tp_current current{ instanceNode->scope };
        if (tp.enter(current)) {
            tp.current->pushStructModifiers(syntaxNode->modifiers);
            tp_struct st{ this, pair };
            st.bindParameters();
            st.bindSupers();
            st.bindBody();
            st.dispose();
            tp.current->popStructModifiers(syntaxNode->modifiers);
            tp.leave(current);
        }
        instanceSymbol->bindStatus.finish();
        traceln("%depth bound   %tptype (with %i#<red> error%c)", &instanceNode->type,
                compiler.errors - errors, compiler.errors - errors == 1 ? "" : "s");
    }
    return instanceSymbol;
}

TpSymbol* tp_site::selectStructInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectStructInstance");
    return nullptr;
}

TpSymbol* tp_site::bindUnionTemplate(TpSymbol *templateSymbol) {
    impl_error(templateSymbol, "bindUnionTemplate");
    return nullptr;
}

TpSymbol* tp_site::selectUnionInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectUnionInstance");
    return nullptr;
}

TpSymbol* tp_site::bindFnTemplate(TpSymbol *symbol) {
    const auto   errors = compiler.errors;
    auto           pair = tp.mk.Function(symbol);
    auto templateSymbol = pair.templateSymbol;
    auto instanceSymbol = pair.instanceSymbol;
    auto   instanceNode = (TpFunction*)instanceSymbol->node;
    auto   templateNode = (TpTemplate*)templateSymbol->node;
    auto     syntaxNode = (FunctionSyntax*)templateNode->syntax;
    Assert(templateSymbol->bindStatus.isIdle());
    Assert(instanceSymbol->bindStatus.isIdle());
    if (instanceSymbol->bindStatus.begin()) {
        traceln("%depth binding %tptype", &instanceNode->type);
        tp_current current{ instanceNode->scope };
        if (tp.enter(current)) {
            tp.current->pushFnModifiers(syntaxNode->modifiers);
            tp_fn fn{ this, pair };
            fn.bindParameters();
            fn.bindReturn();
            fn.bindBody();
            fn.finish();
            fn.dispose();
            tp.current->popFnModifiers(syntaxNode->modifiers);
            tp.leave(current);
        }
        instanceSymbol->bindStatus.finish();
        traceln("%depth bound   %tptype (with %i#<red> error%c)", &instanceNode->type,
                compiler.errors - errors, compiler.errors - errors == 1 ? "" : "s");
    }
    return instanceSymbol;
}

TpSymbol* tp_site::selectFnInstance(TpSymbol *instanceSymbol) {
    auto instanceNode = (TpFunction*)instanceSymbol->node;
    if (arguments.list.length == instanceNode->parameters.length) {
        for (auto i = 0; i < arguments.list.length; i++) {
            auto &argument = arguments.list.items[i];
            auto parameterSymbol = instanceNode->parameters.items[i];
            if (argument.name == parameterSymbol->name) {
                auto parameterNode = (TpParameter*)parameterSymbol->node;
                auto  c = tp.canCast(argument.value, parameterNode->type, tp_cast_reason::ImplicitCast);
                auto ok = c.ok;
                c.dispose();
                if (!ok) {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        }
        return instanceSymbol;
    }
    return nullptr;
}

TpSymbol* tp_site::bindLambdaTemplate(TpSymbol *templateSymbol) {
    const auto   errors = compiler.errors;
    auto   templateNode = (TpTemplate*)templateSymbol->node;
    auto     syntaxNode = (FunctionSyntax*)templateNode->syntax;
    Assert(templateSymbol->bindStatus.isIdle());
    /*  struct `lambda` {
    *       fn ()(this) {
    *           ...lambda body...
    *       }
    *       ...captures...
    *   }
    */
    auto   stPair = tp.mk.LambdaStruct(templateSymbol);
    auto stSymbol = stPair.instanceSymbol;
    auto   stNode = (TpStruct*)stSymbol->node;
    if (stSymbol->bindStatus.begin()) {
        traceln("%depth binding %tptype", &stNode->type);
        tp_current current{ stNode->scope };
        if (tp.enter(current)) {
            auto fnSymbol = tp.mk.LambdaFunction(syntaxNode);
            auto fnNode = (TpFunction*)fnSymbol->node;
            if (fnSymbol->bindStatus.begin()) {
                traceln("%depth binding %tptype", &fnNode->type);
                tp_current current2{ fnNode->scope };
                if (tp.enter(current2)) { // fn ()(this) { ... }
                    tp.current->pushFnModifiers(syntaxNode->modifiers);
                    tp_lambda lambda{ this, syntaxNode, fnSymbol };
                    lambda.bindParameters();
                    tp_fn fn{ this, syntaxNode, fnSymbol };
                    fn.bindReturn();
                    fn.bindBody();
                    fn.finish();
                    fn.dispose();
                    lambda.dispose();
                    tp.current->popFnModifiers(syntaxNode->modifiers);
                    tp.leave(current2);
                }
                fnSymbol->bindStatus.finish();
                traceln("%depth bound   %tptype (with %i#<red> error%c)", &fnNode->type,
                        compiler.errors - errors, compiler.errors - errors == 1 ? "" : "s");
            }
            tp.leave(current);
        }
        stSymbol->bindStatus.finish();
        traceln("%depth bound   %tptype (with %i#<red> error%c)", &stNode->type,
                compiler.errors - errors, compiler.errors - errors == 1 ? "" : "s");
    }
    return stSymbol;
}

TpSymbol* tp_site::selectLambdaInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectLambdaInstance");
    return nullptr;
}

TpSymbol* tp_site::bindExternTemplate(TpSymbol *symbol) {
    const auto   errors = compiler.errors;
    auto        dllPath = ((TpTemplate*)symbol->node)->dllPath;
    auto           pair = tp.mk.Extern(symbol, dllPath);
    auto templateSymbol = pair.templateSymbol;
    auto instanceSymbol = pair.instanceSymbol;
    auto   instanceNode = (TpFunction*)instanceSymbol->node;
    auto   templateNode = (TpTemplate*)templateSymbol->node;
    auto     syntaxNode = (FunctionSyntax*)templateNode->syntax;
    Assert(templateSymbol->bindStatus.isIdle());
    Assert(instanceSymbol->bindStatus.isIdle());
    if (instanceSymbol->bindStatus.begin()) {
        traceln("%depth binding %tptype", &instanceNode->type);
        tp_current current{ instanceNode->scope };
        if (tp.enter(current)) {
            tp.current->pushFnModifiers(syntaxNode->modifiers);
            tp_fn fn{ this, pair };
            fn.bindParameters();
            fn.bindReturn();
            fn.bindBody();
            fn.finish();
            fn.dispose();
            tp.current->popFnModifiers(syntaxNode->modifiers);
            tp.leave(current);
        }
        instanceSymbol->bindStatus.finish();
        traceln("%depth bound   %tptype (with %i#<red> error%c)", &instanceNode->type,
                compiler.errors - errors, compiler.errors - errors == 1 ? "" : "s");
    }
    return instanceSymbol;
}

TpSymbol* tp_site::selectExternInstance(TpSymbol *instanceSymbol) {
    return selectFnInstance(instanceSymbol);
}

TpSymbol* tp_site::bindUrlHandlerTemplate(TpSymbol *templateSymbol) {
    impl_error(templateSymbol, "bindUrlHandlerTemplate");
    return nullptr;
}

TpSymbol* tp_site::selectUrlHandlerInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectUrlHandlerInstance");
    return nullptr;
}

TpSymbol* tp_site::bindHtmlTemplate(TpSymbol *templateSymbol) {
    impl_error(templateSymbol, "bindHtmlTemplate");
    return nullptr;
}

TpSymbol* tp_site::selectHtmlInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectHtmlInstance");
    return nullptr;
}

TpSymbol* tp_site::bindCssTemplate(TpSymbol *templateSymbol) {
    impl_error(templateSymbol, "bindCssTemplate");
    return nullptr;
}

TpSymbol* tp_site::selectCssInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectCsslInstance");
    return nullptr;
}

TpSymbol* tp_site::bindJsTemplate(TpSymbol *templateSymbol) {
    impl_error(templateSymbol, "bindJsTemplate");
    return nullptr;
}

TpSymbol* tp_site::selectJsInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectJsInstance");
    return nullptr;
}

TpSymbol* tp_site::bindJsonTemplate(TpSymbol *templateSymbol) {
    impl_error(templateSymbol, "bindJsonTemplate");
    return nullptr;
}

TpSymbol* tp_site::selectJsonInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectJsonInstance");
    return nullptr;
}

TpSymbol* tp_site::bindSqlTemplate(TpSymbol *templateSymbol) {
    impl_error(templateSymbol, "bindSqlTemplate");
    return nullptr;
}

TpSymbol* tp_site::selectSqlInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectSqlInstance");
    return nullptr;
}

TpSymbol* tp_site::bindBlobTemplate(TpSymbol *templateSymbol) {
    impl_error(templateSymbol, "bindBlobTemplate");
    return nullptr;
}

TpSymbol* tp_site::selectBlobInstance(TpSymbol *instanceSymbol) {
    impl_error(instanceSymbol, "selectBlobInstance");
    return nullptr;
}
} // namespace exy