#pragma once

#include "syntax.h"
#include "tp.h"

namespace exy {
using Type = const TpType&;

using  SyntaxNodes = List<SyntaxNode*>&;

struct Typer;
struct tp_template_instance_pair;
struct tp_mk;
struct tp_site;
struct tp_current;
struct tp_struct;
struct tp_fn;
struct tp_fn_body;
using tp_cast_reason = TpCast::Reason;
using   tp_cast_kind = TpCast::CastKind;
struct tp_cast;
struct tp_lookup;
struct tp_binary;
struct tp_unary_prefix;
struct tp_unary_suffix;
struct tp_parenthesized;
struct tp_bracketed;
struct tp_braced;
struct tp_lambda;
__declspec(selectany) Typer *typer = nullptr;
} // namespace exy

#include "tp_mk.h"
#include "tp_isa.h"
#include "tp_site.h"
#include "tp_current.h"
#include "tp_module.h"
#include "tp_struct.h"
#include "tp_fn.h"
#include "tp_var.h"
#include "tp_cast.h"
#include "tp_lookup.h"
#include "tp_binary.h"
#include "tp_unary.h"
#include "tp_parenthesized.h"
#include "tp_bracketed.h"
#include "tp_braced.h"
#include "tp_lambda.h"

namespace exy {
struct Typer {
    TpTree     &tree;
    Mem        &mem;
    tp_current *current = nullptr;
    tp_isa      isa{};
    tp_mk       mk;
    List<TpNode*>   _thrown;
    TpIndirectTypes _types{};

    TpSymbol *mod_aio = nullptr;
    TpSymbol *sym_aio_OVERLAPPED = nullptr;
    TpSymbol *sym_aio_SRWLOCK    = nullptr;
    TpSymbol *sym_aio_startup = nullptr;
    TpSymbol *sym_aio_shutdown = nullptr;

    TpSymbol *mod_std = nullptr;
    TpSymbol *sym_std_startup = nullptr;
    TpSymbol *sym_std_shutdown = nullptr;
    TpSymbol *sym_std_string  = nullptr;
    TpSymbol *sym_std_String  = nullptr;
    TpSymbol *sym_std_wstring = nullptr;
    TpSymbol *sym_std_WString = nullptr;

    TpSymbol *mod_collections = nullptr;

    Typer();
    void dispose();

    // typer.cpp
    void run();
    void makeBuiltinAliases(SyntaxNode*);

    SourcePos mkPos(SyntaxNode*);

    TpScope* getParentModuleScopeOf(TpScope *scope = nullptr);
    TpSymbol* getTemplateSymbolOf(TpSymbol *instanceSymbol);
    TpTemplate* getTemplateNodeOf(TpSymbol *instanceSymbol);

    // tp_current.cpp
    bool enter(tp_current  &next);
    void leave(tp_current &next);

    // tp_collect.cpp
    void collectTemplates(SyntaxNodes statements);
    TpSymbol* collectTemplate(SyntaxNode *syntax);
    TpSymbol* collectUrlHandler(FunctionSyntax *syntax);
    TpSymbol* collectExtern(FunctionSyntax*);

    // tp_module.cpp
    void bindModule(TpSymbol *moduleSymbol);

    // tp_bind_import.cpp
    void bindImportStatement(ImportSyntax*);
    void bindExportStatement(ImportSyntax*);

    // tp_var.cpp
    void bindGlobal(VariableSyntax*);
    void bindField(VariableSyntax*);
    void bindLocal(VariableSyntax*);

    // tp_apply_modifiers.cpp
    void applyBlockModifiers(TpSymbol*);
    void applyModifiers(SyntaxNode*, TpSymbol*);
    void applyModifier(ModifierSyntax*, TpSymbol*);

    // tp_bind_statement.cpp
    void bindDefineStatement(DefineSyntax*);
    void bindYieldStatement(UnaryPrefixSyntax*);
    void bindAssertStatement(FlowControlSyntax*);
    void bindReturnStatement(FlowControlSyntax*);
    void bindBreakStatement(FlowControlSyntax*);
    void bindContinueStatement(FlowControlSyntax*);
    void bindIfStatement(IfSyntax*);
    void bindForInStatement(ForInSyntax*);
    void bindForStatement(ForSyntax*);
    void bindWhileStatement(WhileSyntax*);

    // tp_bind_expression.cpp
    TpNode* bindCondition(SyntaxNode*); // value != null
    TpNode* bindExpression(SyntaxNode*);
    TpNode* bindStructureExpression(StructureSyntax*);
    TpNode* bindFunctionExpression(FunctionSyntax*);
    TpNode* bindVariableExpression(VariableSyntax*);
    TpNode* bindTernaryExpression(TernarySyntax*);
    TpNode* bindIfExpression(IfExpressionSyntax*);
    TpNode* bindBinaryExpression(BinarySyntax*);
    TpNode* bindUnaryPrefixExpression(UnaryPrefixSyntax*);
    TpNode* bindUnarySuffixExpression(UnarySuffixSyntax*);
    TpNode* bindDotExpression(DotSyntax*);
    TpNode* bindDotExpression(TpNode *base, SyntaxNode *syntax);
    TpNode* bindIndexExpression(TpNode *base, IndexSyntax *syntax);
    TpNode* bindIndexExpression(IndexSyntax*);
    TpNode* bindCallExpression(TpNode *base, CallSyntax *syntax);
    TpNode* bindCallExpression(CallSyntax*);
    TpNode* bindInitializerExpression(TpNode *base, InitializerSyntax*);
    TpNode* bindInitializerExpression(InitializerSyntax*);
    TpNode* bindParenthesizedExpression(ParenthesizedSyntax*);
    TpNode* bindNullExpression(NullSyntax*);
    TpNode* bindVoidExpression(VoidSyntax*);
    TpNode* bindBooleanExpression(BooleanSyntax*);
    TpNode* bindNumberExpression(NumberSyntax*);

    // tp_find.cpp
    TpNode* bindIdentifier(IdentifierSyntax*);
    TpNode* bindIdentifier(TpNode *base, IdentifierSyntax*);

    // tp_cast.cpp
    TpNode* bitCast(SyntaxNode *pos, TpNode *src, const TpType &dst, tp_cast_reason);
    TpNode* cast(SyntaxNode *pos, TpNode *src, const TpType &dst, tp_cast_reason);
    TpNode* cast(SyntaxNode *pos, TpNode *src, const TpType &dst, tp_cast_list&);
    tp_cast_list canCast(TpNode *src, const TpType &dst, tp_cast_reason);
    tp_cast_list canBitCast(TpNode *src, const TpType &dst, tp_cast_reason);
    TpType upperBound(Type lhs, Type rhs, Tok op = Tok::Unknown);

    // typer.cpp
    template<typename T, typename U>
    TpNode* throwAway(T *a, U *b) {
        throwAway(a);
        throwAway(b);
        return nullptr;
    }

    template<typename T, typename U, typename V>
    TpNode* throwAway(T *a, U *b, V *c) {
        throwAway(a);
        throwAway(b);
        throwAway(c);
        return nullptr;
    }

    template<typename T>
    T* throwAway(T *node) {
        if (node != nullptr) {
            _thrown.append(node);
        }
        return nullptr;
    }

    template<typename T>
    void throwAway(List<T*> &nodes) {
        for (auto i = 0; i < nodes.length; i++) {
            throwAway(nodes.items[i]);
        }
        nodes.dispose();
    }


};

#define dup_error(pos, dup)        diagnostic("DuplicateName", pos, "identifier %s#<red> already defined in scope as %tpk: %tptype", dup->name, dup->node->kind, &dup->node->type)
#define type_error(pos, msg, ...)  diagnostic("Type", pos, msg, __VA_ARGS__)
#define cast_error(pos, result)    diagnostic("Cast", pos, "%c cast failed: %tptype → %tptype", result.reason == TpCast::Reason::ImplicitCast ? "implicit" : "explicit", &result.src, &result.dst)
} // namespace exy