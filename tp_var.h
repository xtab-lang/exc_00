#pragma once

namespace exy {
struct tp_var {
    Typer &tp;
    VariableSyntax *syntax;

    tp_var(VariableSyntax *syntax);
    void dispose();

    void bindGlobal();
    void bindField();
    void bindLocal();

private:
    void bind(TpKind);
};
}