#pragma once

namespace exy {
struct tp_module {
    Typer     &tp;
    TpSymbol  *moduleSymbol;

    tp_module(TpSymbol *moduleSymbol);
    void dispose();

    void bind();

private:
    void bindStatements(SyntaxNodes);
    void bindStatement(SyntaxNode*);
    void findAndBindMain();
    void bindMain(TpSymbol *templateSymbol);
};
} 