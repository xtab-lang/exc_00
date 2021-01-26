//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-16
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef STX2AST_IMPORT_H_
#define STX2AST_IMPORT_H_

namespace exy {
namespace stx2ast_pass {
struct Importer {
    Typer  &tp;

    Importer(Typer *tp) : tp(*tp) {}

    void visit(SyntaxImportOrExport*);
private:
    AstModule* findSourceModule(SyntaxNode*);

    AstSymbol* findSymbol(SyntaxNode*, AstScope*);
    AstSymbol* findSymbol(SyntaxNode*);

    AstSymbol* findSymbol(Pos, Identifier);

    void createImport(SyntaxImportOrExport*, AstSymbol*, Identifier);
    void createExport(SyntaxImportOrExport*, AstSymbol*, Identifier);

    AstSymbol* checkForSelfImportOrExport(Pos pos, AstSymbol *symbol);
};
} // namespace stx2ast_pass
} // namespace exy

#endif // STX2AST_IMPORT_H_