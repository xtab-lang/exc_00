//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef STX2AST_MODIFIERS_H_
#define STX2AST_MODIFIERS_H_

namespace exy {
namespace stx2ast_pass {
struct Modifiers {
#define ZM(zName, zText) static bool is##zName(SyntaxNode*);
    DeclareModifiers(ZM)
#undef ZM

    static bool validateImportOrExportModifiers(SyntaxNode*);
    static bool validateVariableModifiers(SyntaxNode*);
};
} // namespace stx2ast_pass
} // namespace exy

#endif // STX2AST_MODIFIERS_H_