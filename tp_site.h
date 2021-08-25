#pragma once

namespace exy {
struct tp_argument {
    SyntaxNode *syntax; // SyntaxNode | NameValueSyntax
    Identifier  name  = nullptr;
    TpNode     *value = nullptr;
    INT         parameterIndex = -1;

    tp_argument(SyntaxNode*);
    void dispose();
    auto isReceiver() { return name == ids.kw_this; }
    auto isNotReceiver() { return name != ids.kw_this; }
};

struct tp_argument_list {
    EnclosedSyntax   *syntax{}; // ParenthesizedSyntax | BracketedSyntax | AngledSyntax | BracedSyntax
    List<tp_argument> list{};

    void dispose();
    bool set(TpNode *receiver, EnclosedSyntax*, FunctionSyntax *with);
    tp_argument& append(SyntaxNode *pos, TpNode *value);
    INT indexOf(Identifier name);
};

//----------------------------------------------------------
struct tp_parameter {
    SyntaxNode  *syntax; // VariableSyntax | RestParameterSyntax
    SyntaxNode  *modifiers = nullptr;
    Identifier   name = nullptr;
    TpType       type{};
    TpNode      *defaultValue  = nullptr;
    INT          argumentIndex = -1;

    tp_parameter(SyntaxNode*);
    void dispose();

    auto isReceiver() { return name == ids.kw_this; }
    auto isNotReceiver() { return name != ids.kw_this; }
    bool isaRequiredParameter() const;
    bool isaDefaultParameter() const;
    bool isaRestParameter() const;
    auto isNotARestParameter() const { return !isaRestParameter(); }
};

struct tp_parameter_list {
    EnclosedSyntax    *syntax{};
    List<tp_parameter> list{};

    void dispose();
    bool set(TpScope *scope, ParenthesizedSyntax*);
    bool set(TpScope *scope, AngledSyntax*);
    bool set(tp_argument_list &arguments);
    bool prependThis(Type type);
    INT indexOf(Identifier name);
};

//----------------------------------------------------------
struct tp_site {
    Typer      &tp;
    tp_site    *prev;
    SyntaxNode *pos;
    tp_argument_list  arguments{};
    tp_parameter_list parameters{};

    tp_site(SyntaxNode *pos);
    void dispose();

    void synchronize(TpNode*);
private:
    void lock(TpNode *receiver, TpNode *fnName);
    void unlock(TpNode *receiver, TpNode *fnName);
public:

    TpNode* callDispose(TpNode *receiver, TpNode *with);
private:
    TpNode* callDisposeOverload(TpNode *receiver, TpNode *with, TpNode *fnName, TpSymbol *overloadSymbol);
    TpNode* callDisposeFunctionTemplate(TpNode *receiver, TpNode *with, TpNode *fnName, TpSymbol *templateSymbol);
public:

    TpNode* tryCallOperator(TpNode *lhs, Tok op, TpNode *rhs);
    TpNode* tryCallOperator(Tok op, TpNode *value);
    TpNode* tryCallIndexer(TpNode *receiver);
    TpSymbol* createAsyncMain(FunctionSyntax *pos, TpSymbol *instanceSymbol);

    TpSymbol* selectInstance(TpSymbol *templateSymbol);
    TpSymbol* bindTemplate(TpSymbol *templateSymbol);
private:
#define ZM(zName, zText) \
    TpSymbol* bind##zName##Template(TpSymbol *templateSymbol); \
    TpSymbol* select##zName##Instance(TpSymbol *instanceSymbol);
    DeclareStructureTypeKeywords(ZM)
#undef ZM
#define ZM(zName, zText) \
    TpSymbol* bind##zName##Template(TpSymbol *templateSymbol); \
    TpSymbol* select##zName##Instance(TpSymbol *instanceSymbol);
    DeclareFunctionTypeKeywords(ZM)
#undef ZM
};
} // namespace exy