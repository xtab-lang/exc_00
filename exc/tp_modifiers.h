//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_MODIFIERS_H_
#define TP_MODIFIERS_H_

namespace exy {
namespace typ_pass {
struct Modifiers {
#define ZM(zName, zText) static bool is##zName(SyntaxNode*);
    DeclareModifiers(ZM)
#undef ZM

    static bool validateImportOrExportModifiers(SyntaxNode*);
    static bool validateVariableModifiers(SyntaxNode*);
};
} // namespace typ_pass
} // namespace exy

#endif // TP_MODIFIERS_H_