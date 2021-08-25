#pragma once

namespace exy {
struct tp_lambda {
    Typer          &tp;
    tp_site        &site;
    FunctionSyntax *fnSyntax;
    TpSymbol       *fnSymbol;
    TpFunction     *fnNode;

    tp_lambda(tp_site *site, FunctionSyntax *syntax, TpSymbol *fnSymbol);
    void dispose();

    void bindParameters();
};
} // namespace exy
