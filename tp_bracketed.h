#pragma once

namespace exy {
struct tp_bracketed {
    Typer      &tp;
    SyntaxNode *pos;
    SyntaxNode *nameSyntax;
    BracketedSyntax *argumentsSyntax;

    tp_bracketed(IndexSyntax *syntax);
    void dispose();

    TpNode* bind(TpNode *base);

private:
    TpNode* bindIndexedTypename(TpTypeName *base);

    TpNode* bindList(TpTypeName *tpname);
    TpSymbol* selectList(TpSymbol *templateSymbol);
    bool setSiteArgumentsAndParameters(TpSymbol *templateSymbol, StructureSyntax *syntaxNode);
    TpSymbol* selectListInstance(TpSymbol *templateSymbol);

    TpNode* bindFixedArray(TpTypeName *tpname, TpConstExpr *length);

private:
    TpNode* bindIndexer(TpNode*);
};
} // namespace exy