#pragma once

namespace exy {
struct tp_fn_body {
    Typer &tp;

    tp_fn_body();
    void dispose() {}

    void bind(SyntaxNode*);
    void bind(List<SyntaxNode*>&);
private:
    void bindStatement(SyntaxNode*);
    void bindBlockWithArguments(BlockSyntax*);
};

//----------------------------------------------------------
struct tp_if_block {
    Typer    &tp;
    IfSyntax *syntax;

    tp_if_block(IfSyntax*);
    void dispose() {}

    void bind();
};

//----------------------------------------------------------
struct tp_forin_loop {
    Typer       &tp;
    ForInSyntax *syntax;

    tp_forin_loop(ForInSyntax*);
    void dispose() {}

    void bind();
private:
    TpNode* bindArrayLoop(TpNode *expr, TpSymbol *symbol);
    TpNode* bindStructLoop(TpNode *expr, TpSymbol *stSymbol);
    TpNode* bindResumableLoop(TpNode *expr, TpSymbol *symbol);
};

//----------------------------------------------------------
struct tp_for_loop {
    Typer     &tp;
    ForSyntax *syntax;

    tp_for_loop(ForSyntax*);
    void dispose() {}

    void bind();
};

//----------------------------------------------------------
struct tp_while_loop {
    Typer       &tp;
    WhileSyntax *syntax;

    tp_while_loop(WhileSyntax*);
    void dispose() {}

    void bind();
};

//----------------------------------------------------------
struct tp_fn {
    Typer          &tp;
    tp_site        &site;
    FunctionSyntax *fnSyntax;
    TpSymbol       *fnSymbol;
    TpFunction     *fnNode;

    tp_fn(tp_site *, tp_template_instance_pair&);
    tp_fn(tp_site *, FunctionSyntax *fnSyntax, TpSymbol *fnSymbol);
    void dispose();

    void bindParameters();
    void bindReturn();
    void bindBody();
    void finish();

private:
    void bindResumableBody();
};
} // namespace exy