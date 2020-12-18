//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-16
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_IMPORT_H_
#define TP_IMPORT_H_

namespace exy {
namespace typ_pass {
//--Begin forward declarations
struct Typer;
//----End forward declarations

struct Importer {
    using Decl = SyntaxImportOrExport*;
    Typer  &tp;

    Importer(Typer *tp) : tp(*tp) {}

    void visit(Decl);
private:
    AstModule* findSourceModule(SyntaxNode*);

    AstSymbol* findSymbol(SyntaxNode*, AstScope*);
    AstSymbol* findSymbol(SyntaxNode*);

    AstSymbol* findSymbol(Pos, Identifier);

    void createImport(Decl, AstSymbol*, Identifier);
    void createExport(Decl, AstSymbol*, Identifier);
};
} // namespace typ_pass
} // namespace exy

#endif // TP_IMPORT_H_