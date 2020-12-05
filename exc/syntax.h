//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-02
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SYNTAX_H
#define SYNTAX_H

namespace exy {
//--Begin forward declarations
struct SourceToken;
struct SourceFile;

struct SyntaxTree;
    struct SyntaxNode;
    struct SyntaxFile;
    struct SyntaxModule;
    struct SyntaxImport;
    struct SyntaxExport;
    struct SyntaxCommaList;
    struct SyntaxDotExpression;
    struct SyntaxIdentifier;

using Pos = const SourceToken&;
//----End forward declarations


#define DeclareSyntaxNodes(ZM)   \
    ZM(File)                     \
    ZM(Module)                   \
    ZM(Import)                   \
    ZM(Export)                   \
    ZM(CommaList)                \
    ZM(DotExpression)            \
    ZM(Identifier)

enum class SyntaxKind {
#define ZM(zName) zName,
    DeclareSyntaxNodes(ZM)
#undef ZM
};
//------------------------------------------------------------------------------------------------
struct SyntaxTree {
    Mem               mem;
    List<SyntaxFile*> files;

    void dispose();
};

struct SyntaxNode {
    using Kind = SyntaxKind;

    Pos        pos;
    SyntaxKind kind;

    SyntaxNode(Pos pos, Kind kind) : pos(pos), kind(kind) {}
    virtual void dispose() {}
};

struct SyntaxFile : SyntaxNode {
    List<SyntaxNode*> nodes;

    SyntaxFile(Pos pos) : SyntaxNode(pos, Kind::File) {}
    void dispose() override;
    const SourceFile& sourceFile() const;
    const List<SourceToken>& tokens() const;
};

struct SyntaxModule : SyntaxNode {
    SyntaxNode *name;

    SyntaxModule(Pos pos) : SyntaxNode(pos, Kind::Module) {}
    void dispose() override;
};

struct SyntaxImportOrExport : SyntaxNode {
    SyntaxNode *name;
    SyntaxNode *from;
    SyntaxNode *as;

    SyntaxImportOrExport(Pos pos, Kind kind) : SyntaxNode(pos, kind) {}
    void dispose() override;
};

struct SyntaxImport : SyntaxImportOrExport {
    SyntaxImport(Pos pos) : SyntaxImportOrExport(pos, Kind::Import) {}
};

struct SyntaxExport : SyntaxImportOrExport {
    SyntaxExport(Pos pos) : SyntaxImportOrExport(pos, Kind::Export) {}
};

struct SyntaxCommaList : SyntaxNode {
    List<SyntaxNode*> nodes;

    SyntaxCommaList(Pos pos, List<SyntaxNode*> &nodes) : SyntaxNode(pos, Kind::CommaList), nodes(nodes) {}
    void dispose() override;
};

struct SyntaxDotExpression : SyntaxNode {
    SyntaxNode *lhs;
    Pos         dot;
    SyntaxNode *rhs;

    SyntaxDotExpression(Pos pos, SyntaxNode *lhs, Pos dot, SyntaxNode *rhs) : 
        SyntaxNode(pos, Kind::DotExpression), lhs(lhs), dot(dot), rhs(rhs) {}
    void dispose() override;
};

struct SyntaxIdentifier : SyntaxNode {
    Identifier value;

    SyntaxIdentifier(Pos pos, Identifier value) : SyntaxNode(pos, Kind::Identifier), value(value) {}
};
//------------------------------------------------------------------------------------------------
struct SyntaxFileProvider : WorkProvider<SyntaxFile> {
    List<SyntaxFile*> &files;
    int                pos{};

    SyntaxFileProvider();
    void dispose();
    bool next(List<SyntaxFile*> &batch);
};

namespace syn_pass {
bool run();
} // namespace syn_pass
} // namespace exy

#endif // SYNTAX_H