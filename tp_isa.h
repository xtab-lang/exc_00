#pragma once

namespace exy {
struct tp_isa {
    static TpTypeNode* TypeNode(TpNode*);
    static TpSymbol* TypeSymbol(TpSymbol*);

    static TpAliasNode* AliasNode(TpNode*);
    static TpSymbol* AliasSymbol(TpSymbol*);

    static TpValueNode* ValueNode(TpNode*);
    static TpSymbol* ValueSymbol(TpSymbol*);

    static TpSymbol* ModuleScope(TpScope*);

    static TpConstExpr* Null(TpNode*);

    static TpSymbol* ResumableStruct(Type);
    static bool PassByValue(Type);
    static auto PassByReference(Type type) { return !PassByValue(type); }
    static auto CaptureByReference(Type type) { return type.isNotAReference(); }

    static TpSymbol* LambdaStructSymbol(TpSymbol*);
    static TpStruct* LambdaStruct(TpSymbol*);
    static TpStruct* LambdaStruct(TpNode*);

    static TpSymbol* LambdaFunctionSymbol(TpSymbol*);
    static TpFunction* LambdaFunction(TpSymbol*);
    static TpFunction* LambdaFunction(TpNode*);

    static TpSymbol* LambdaTemplateSymbol(TpSymbol*);
    static TpTemplate* LambdaTemplate(TpSymbol*);
    static TpTemplate* LambdaTemplate(TpNode*);


    static TpSymbol* LocalOrParameterSymbol(TpSymbol*);
    static TpValueNode* LocalOrParameter(TpSymbol*);
    static TpValueNode* LocalOrParameter(TpNode*);

    static TpName* Name(TpNode*);
    static TpFunction* MemberFunction(TpFunction*);
    static TpTemplate* FunctionTemplate(TpSymbol*);

    static TpValueName* This(TpNode*);

    static SyntaxNode* BlockMaker(SyntaxNode*);

    //------------------------------------------------------
#define ZM(zName) static TpSymbol* zName##Symbol(TpSymbol *symbol) { return symbol == nullptr ? nullptr : symbol->node->kind == TpKind::zName ? symbol : nullptr; }
    DeclareTpSymbolNodes(ZM)
#undef ZM

    #define ZM(zName) static Tp##zName* zName(TpSymbol *symbol) { return symbol == nullptr ? nullptr : symbol->node->kind == TpKind::zName ? ((Tp##zName*)symbol->node) : nullptr; }
    DeclareTpSymbolNodes(ZM)
    #undef ZM

    #define ZM(zName) static Tp##zName* zName(TpNode *node) { return node == nullptr ? nullptr : node->kind == TpKind::zName ? ((Tp##zName*)node) : nullptr; }
        DeclareTpExpressionNodes(ZM)
    #undef ZM

    //------------------------------------------------------
#define ZM(zName) static zName##Syntax* zName##Syntax(SyntaxNode *syntax) { return syntax == nullptr ? nullptr : syntax->kind == SyntaxKind::zName ? (exy::zName##Syntax*)syntax : nullptr; }
    DeclareSyntaxNodes(ZM)
#undef ZM

    //------------------------------------------------------
#define ZM(zName, zText) static auto kw##zName(Keyword kw) { return kw == Keyword::zName; }
    DeclareKeywords(ZM)
#undef ZM
#define ZM(zName, zText) \
    static auto kw##zName(Keyword kw) { return kw == Keyword::zName; }\
    static exy::ModifierSyntax* kw##zName(SyntaxNode *n) { return n == nullptr ? nullptr : n->kind == SyntaxKind::Modifier ? ((exy::ModifierSyntax*)n)->is(Keyword::zName) : n->kind == SyntaxKind::ModifierList ? ((exy::ModifierListSyntax*)n)->contains(Keyword::zName) : nullptr; }
    DeclareModifiers(ZM)
#undef ZM
#define ZM(zName, zText) static auto kw##zName(Keyword kw) { return kw == Keyword::zName; }
    DeclareUserDefinedTypeKeywords(ZM)
#undef ZM
#define ZM(zName, zText) static auto kw##zName(Keyword kw) { return kw == Keyword::zName; }
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
#define ZM(zName, zText) static auto kw##zName(Keyword kw) { return kw == Keyword::zName; }
    DeclareCompileTimeKeywords(ZM)
#undef ZM
};
} // namespace exy