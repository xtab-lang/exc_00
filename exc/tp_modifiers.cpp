//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(token, msg, ...) print_error("Modifier", token, msg, __VA_ARGS__)

namespace exy {
namespace stx2ast_pass {
#define ZM(zName, zText) bool Modifiers::is##zName(SyntaxNode *syntax) { \
    if (syntax->kind == SyntaxKind::Modifier) return ((SyntaxModifier*)syntax)->value == Keyword::zName; \
    if (syntax->kind == SyntaxKind::Modifiers) return ((SyntaxModifiers*)syntax)->contains(Keyword::zName); \
    err(syntax, "not a modifier"); \
    return false;\
}
    DeclareModifiers(ZM)
#undef ZM

bool Modifiers::validateImportOrExportModifiers(SyntaxNode *syntax) {
    auto errors = comp.errors;
    if (syntax) {
        if (syntax->kind == SyntaxKind::Modifier) {
            auto node = (SyntaxModifier*)syntax;
            err(syntax, "modifier %kw#<yellow> not allowed on a variable", node->value);
        } else if (syntax->kind == SyntaxKind::Modifiers) {
            auto &list = ((SyntaxModifiers*)syntax)->nodes;
            for (auto i = 0; i < list.length; ++i) {
                validateImportOrExportModifiers(list.items[i]);
            }
        } else {
            err(syntax, "not a modifier");
        }
    }
    return errors == comp.errors;
}

bool Modifiers::validateVariableModifiers(SyntaxNode *syntax) {
    auto errors = comp.errors;
    if (syntax) {
        if (syntax->kind == SyntaxKind::Modifier) {
            auto node = (SyntaxModifier*)syntax;
            if (node->value > Keyword::Var) {
                err(syntax, "modifier %kw#<yellow> not allowed on a variable", node->value);
            }
        } else if (syntax->kind == SyntaxKind::Modifiers) {
            auto &list = ((SyntaxModifiers*)syntax)->nodes;
            for (auto i = 0; i < list.length; ++i) {
                validateVariableModifiers(list.items[i]);
            }
        } else {
            err(syntax, "not a modifier");
        }
    }
    return errors == comp.errors;
}
} // namespace stx2ast_pass
} // namespace exy