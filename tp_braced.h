#pragma once

namespace exy {
struct tp_braced {
    Typer        &tp;
    SyntaxNode   *pos;
    SyntaxNode   *nameSyntax;
    BracedSyntax *argumentsSyntax;

    tp_braced(InitializerSyntax *syntax);
    void dispose();

    TpNode* bind(TpNode *base);

private:
    TpNode* bindTypeNameWithBraced(TpTypeName *name);
};
} // namespace exy