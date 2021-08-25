#pragma once

namespace exy {
struct tp_current {
    ExternBlockSyntax *ext = nullptr;
    List<SyntaxNode*> modifiers{};
    Typer         &tp;
    tp_current    *prev;
    TpScope       *scope;
    tp_site       *site  = nullptr;
    INT           depth;

    tp_current(TpScope *scope);
    void dispose();

    ExternBlockSyntax* pushExternBlock(ExternBlockSyntax*);
    void popExternBlock(ExternBlockSyntax *prev, ExternBlockSyntax *current);

    void pushBlockModifiers(SyntaxNode*);
    void popBlockModifiers(SyntaxNode*);

    void pushStructModifiers(SyntaxNode*);
    void popStructModifiers(SyntaxNode*);

    void pushFnModifiers(SyntaxNode*);
    void popFnModifiers(SyntaxNode*);

    TpLabel* isaLoop();
    bool isaLocalsScope();
    bool isStatic();

    TpSymbol* lambdaFunction();
    TpSymbol* function();
};
} // namespace exy