#include "pch.h"
#include "typer.h"

namespace exy {
tp_lambda::tp_lambda(tp_site *site, FunctionSyntax *syntax, TpSymbol *fnSymbol)
    : tp(*typer), site(*site), fnSyntax(syntax), fnSymbol(fnSymbol), fnNode((TpFunction*)fnSymbol->node) {}

void tp_lambda::dispose() {}

void tp_lambda::bindParameters() {
    Assert(fnNode->parameters.length == 1);
    if (site.parameters.list.isEmpty()) {
        return;
    }
    for (auto i = 0; i < site.parameters.list.length; i++) {
        auto &parameter = site.parameters.list.items[i];
        Assert(parameter.syntax != nullptr && parameter.name != nullptr && parameter.type.isKnown());
        if (parameter.isReceiver()) {
            auto dup = tp.current->scope->contains(parameter.name);
            Assert(dup->node->type == parameter.type);
            continue;
        }
        if (auto dup = tp.current->scope->contains(parameter.name)) {
            dup_error(parameter.syntax, dup);
        } else if (auto parameterSymbol = tp.mk.Parameter(parameter.syntax, parameter.name, parameter.type)) {
            tp.applyModifiers(parameter.modifiers, parameterSymbol);
            fnNode->parameters.append(parameterSymbol);
        }
    }
}
} // namespace exy