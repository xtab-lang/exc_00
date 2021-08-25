#include "pch.h"
#include "typer.h"

#define call_error(pos, msg, ...) diagnostic("Call", pos, msg, __VA_ARGS__)

namespace exy {
tp_parenthesized::tp_parenthesized(CallSyntax *syntax) 
    : tp(*typer), pos(syntax), nameSyntax(syntax->name), argumentsSyntax(syntax->arguments)
    , withSyntax(syntax->with) {}

void tp_parenthesized::dispose() {
}

TpNode* tp_parenthesized::bind(TpNode *base) {
    if (nameSyntax == nullptr) {
        Assert(0);
    }
    tp_site site{ pos };
    TpNode *result = nullptr;
    
    if (base == nullptr) {
        if (auto callee = tp.bindExpression(nameSyntax)) {
            result = call(callee);
        }
    } else if (tp.isa.TypeName(base)) {
        if (auto callee = tp.bindDotExpression(base, nameSyntax)) {
            result = call(callee);
        }
        tp.throwAway(base);
    } else if (auto callee = tp.bindDotExpression(base, nameSyntax)) {
        result = call(/* receiver = */ base, callee);
    }
    site.dispose();
    return result;
}

TpNode* tp_parenthesized::call(TpNode *callee) {
    callee = tp.mk.DereferenceIfReference(nameSyntax, callee);
    if (auto symbol = callee->type.isDirect()) {
        return call(/* receiver = */ nullptr, callee, symbol);
    }
    call_error(pos, "cannot call %tpk: %tptype", callee->kind, &callee->type);
    return tp.throwAway(callee);
}

TpNode* tp_parenthesized::call(TpNode *receiver, TpNode *callee) {
    if (auto name = tp.isa.TypeName(callee)) {
        if (auto symbol = callee->type.isDirect()) {
            return call(receiver, name, symbol);
        }
        return tp.throwAway(receiver);
    }
    if (tp.isa.ValueName(callee) || tp.isa.FieldName(callee)) { // Local | Parameter | Global | Field
        auto name = (TpName*)callee;
        if (auto symbol = name->type.isDirect()) {
            return call(receiver, name, symbol);
        }
    }
    call_error(pos, "cannot call %tpk: %tptype", callee->kind, &callee->type);
    return tp.throwAway(receiver, callee);
}

TpNode* tp_parenthesized::call(TpNode *receiver, TpNode *name, TpSymbol *callee) {
    TpNode  *result = nullptr;
    auto calleeNode = callee->node;
    switch (calleeNode->kind) {
        case TpKind::OverloadSet: {
            result = callOverloadSet(receiver, name, callee);
        } break;

        case TpKind::Template: {
            result = callTemplate(receiver, name, callee);
        } break;

        case TpKind::Function: {
            result = callFunction(receiver, name, callee);
        } break;

        case TpKind::Struct: {
            //result = selectStruct(calleeSymbol);
            Assert(0);
        } break;
        default: {
            type_error(nameSyntax, "cannot call %tpk: %tptype", calleeNode->kind, &calleeNode->type);
            tp.throwAway(receiver, name);
        } break;
    }
    return result;
}

tp_parenthesized::Match tp_parenthesized::matchFunctionTemplateByArity(TpNode *receiver, TpArity &arity) {
    auto   arguments = 0;
    auto putReceiver = false;
    if (arity.hasThis && receiver != nullptr) {
        ++arguments;
        putReceiver = true;
    }
    if (auto list = tp.isa.CommaSeparatedSyntax(argumentsSyntax->value)) {
        arguments += list->nodes.length;
    } else if (argumentsSyntax->value != nullptr) {
        ++arguments;
    }
    if (withSyntax != nullptr) {
        ++arguments;
    }
    if (arguments < arity.required) {
        return { /* putReceiver = */ false, /* isOk = */ false};
    }
    if (arguments == arity.required) {
        return { putReceiver, /* isOk = */ true};
    }
    if (arity.varags) {
        return { putReceiver, /* isOk = */ true };
    }
    if (arguments <= arity.required + arity.defaults) {
        return { putReceiver, /* isOk = */ true };
    }
    return { /* putReceiver = */ false, /* isOk = */ false };
}

tp_parenthesized::Match tp_parenthesized::matchFunctionInstanceByArity(TpNode *receiver, 
                                                                       TpFunction *fnNode) {
    auto   arguments = 0;
    auto putReceiver = false;
    if (tp.isa.MemberFunction(fnNode) && receiver != nullptr) {
        ++arguments;
        putReceiver = true;
    }
    if (auto list = tp.isa.CommaSeparatedSyntax(argumentsSyntax->value)) {
        arguments += list->nodes.length;
    } else if (argumentsSyntax->value != nullptr) {
        ++arguments;
    }
    if (withSyntax != nullptr) {
        ++arguments;
    }
    if (arguments == fnNode->parameters.length) {
        return { putReceiver, /* isOk = */ true };
    }
    return { /* putReceiver = */ false, /* isOk = */ false };
}

TpNode* tp_parenthesized::callOverloadSet(TpNode *receiver, TpNode *name, TpSymbol *callee) {
    auto ovNode = (TpOverloadSet*)callee->node;
    for (auto i = 0; i < ovNode->list.length; i++) {
        auto templateSymbol = ovNode->list.items[i];
        auto   templateNode = (TpTemplate*)templateSymbol->node;
        auto templateSyntax = templateNode->syntax;
        if (tp.isa.FunctionSyntax(templateSyntax)) {
            auto match = matchFunctionTemplateByArity(receiver, templateNode->arity);
            if (match.isOk) {
                if (!match.putReceiver) {
                    receiver = tp.throwAway(receiver);
                }
                return callTemplate(receiver, name, templateSymbol);
            }
        } else {
            call_error(pos, "call syntax cannot choose from templates of %tptype", &ovNode->type);
            return nullptr;
        }
    }
    call_error(pos, "no template of %tptype matches the arguments supplied", &ovNode->type);
    return nullptr;
}

TpNode* tp_parenthesized::callTemplate(TpNode *receiver, TpNode *name, TpSymbol *callee) {
    auto &site = *tp.current->site;
    auto templateNode = (TpTemplate*)callee->node;
    if (auto fnSyntax = tp.isa.FunctionSyntax(templateNode->syntax)) {
        if (tp.isa.LambdaTemplate(callee)) {
            Assert(receiver == nullptr && tp.isa.Dereference(name));
            receiver = tp.mk.ReferenceOf(nameSyntax, name);
            if (site.arguments.set(/* receiver = */ receiver, argumentsSyntax, withSyntax)) {
                if (site.parameters.set(callee->scope, fnSyntax->parameters)) {
                    if (site.parameters.prependThis(receiver->type)) {
                        if (site.parameters.set(site.arguments)) {
                            if (auto selected = site.selectInstance(callee)) {
                                auto   stNode = (TpStruct*)selected->node;
                                return mkCall(name, stNode->scope->contains(ids.kw_open_close_parens));
                            } else if (selected = site.bindTemplate(callee)) {
                                auto stNode = (TpStruct*)selected->node;
                                return mkCall(name, stNode->scope->contains(ids.kw_open_close_parens));
                            }
                        }
                    }
                }
            }
            Assert(0);
        } else if (site.arguments.set(receiver, argumentsSyntax, withSyntax)) {
            if (site.parameters.set(callee->scope, fnSyntax->parameters)) {
                if (site.parameters.set(site.arguments)) {
                    if (auto selected = site.selectInstance(callee)) {
                        return mkCall(name, selected);
                    } else if (selected = site.bindTemplate(callee)) {
                        return mkCall(name, selected);
                    }
                }
            }
        } else {
            call_error(pos, "cannot call %tptype because it is not a function", &callee->node->type);
        }
    }
    return tp.throwAway(name);
}

TpNode* tp_parenthesized::callFunction(TpNode *receiver, TpNode *name, TpSymbol *callee) {
    auto  &site = *tp.current->site;
    auto fnNode = (TpFunction*)callee->node;
    auto  match = matchFunctionInstanceByArity(receiver, fnNode);
    if (match.isOk) {
        if (!match.putReceiver) {
            receiver = tp.throwAway(receiver);
        }
        if (site.arguments.set(receiver, argumentsSyntax, withSyntax)) {
            Assert(site.arguments.list.isEmpty());
            Assert(fnNode->parameters.isEmpty());
            return mkCall(name, callee);
        }
        return tp.throwAway(name);
    }
    call_error(pos, "cannot call %tptype", &callee->node->type);
    return tp.throwAway(receiver, name);
}

TpNode* tp_parenthesized::mkCall(TpNode *name, TpSymbol *callee) {
    auto  &site = *tp.current->site;
    auto fnNode = (TpFunction*)callee->node;
    // Recursion.
    if (fnNode->fnreturn.isUnknown()) {
        type_error(site.pos, "are you recursing at %tptype", &fnNode->type);
        return tp.throwAway(name);
    }
    return tp.mk.FunctionCallFromSite(name, callee);
}
} // namespace exy