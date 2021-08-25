#pragma once

namespace exy {
struct tp_parenthesized {
    Typer          &tp;
    SyntaxNode     *pos;
    SyntaxNode     *nameSyntax;
    FunctionSyntax *withSyntax = nullptr;
    ParenthesizedSyntax *argumentsSyntax;

    tp_parenthesized(CallSyntax *syntax);
    void dispose();

    TpNode* bind(TpNode *base);

private:
    TpNode* call(TpNode *callee);
    TpNode* call(TpNode *receiver, TpNode *callee);

    TpNode* call(TpNode *receiver, TpNode *name, TpSymbol *callee);

    struct Match {
        bool putReceiver;
        bool isOk;
    };
    Match matchFunctionTemplateByArity(TpNode *receiver, TpArity &arity);
    Match matchFunctionInstanceByArity(TpNode *receiver, TpFunction *fnNode);

    TpNode* callOverloadSet(TpNode *receiver, TpNode *name, TpSymbol *callee);
    TpNode* callTemplate(TpNode *receiver, TpNode *name, TpSymbol *callee);
    TpNode* callFunction(TpNode *receiver, TpNode *name, TpSymbol *callee);

    TpNode* mkCall(TpNode *name, TpSymbol *callee);
};
} // namespace exy