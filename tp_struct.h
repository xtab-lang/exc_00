#pragma once

namespace exy {
struct tp_struct {
    Typer      &tp;
    tp_site    &site;
    StructureSyntax *stSyntax;
    TpSymbol        *stSymbol;
    TpStruct        *stNode;

    tp_struct(tp_site *site, tp_template_instance_pair &pair);
    void dispose();

    void bindParameters();
    void bindSupers();
    void bindBody();

private:
    void bindStatement(SyntaxNode*);
    void bindVariable(VariableSyntax*);
};
} // namespace exy