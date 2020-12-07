//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-02
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SYNTAX_H
#define SYNTAX_H

namespace exy {
//--Begin forward declarations
enum class Keyword;
struct SourceToken;
struct SourceFile;

struct SyntaxTree;
    struct SyntaxNode;
    struct SyntaxEmpty;
    struct SyntaxModifier;
    struct SyntaxModifiers;
    struct SyntaxFile;
    struct SyntaxModule;
    struct SyntaxImport;
    struct SyntaxExport;
    struct SyntaxCommaList;
    struct SyntaxDotExpression;
    struct SyntaxBoolean;
    struct SyntaxIdentifier;

using Pos = const SourceToken&;
//----End forward declarations


#define DeclareSyntaxNodes(ZM)   \
    ZM(Empty)                    \
    ZM(Modifier)                 \
    ZM(Modifiers)                \
    ZM(File)                     \
    ZM(Module)                   \
    ZM(Import)                   \
    ZM(Export)                   \
    ZM(CommaList)                \
    ZM(DotExpression)            \
    ZM(Boolean)                  \
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
    using Node = SyntaxNode*;

    Pos  pos;
    Node modifiers;
    Kind kind;

    SyntaxNode(Pos pos, Kind kind, Node modifiers) : pos(pos), kind(kind), modifiers(modifiers) {}
    virtual void dispose();
};

struct SyntaxEmpty : SyntaxNode {
    SyntaxEmpty(Pos pos, Node modifiers) : SyntaxNode(pos, Kind::Empty, modifiers) {}
};

struct SyntaxModifier : SyntaxNode {
    Keyword value;

    SyntaxModifier(Pos pos, Keyword value) : SyntaxNode(pos, Kind::Modifier, nullptr),
        value(value) {}
};

struct SyntaxModifiers : SyntaxNode {
    List<SyntaxModifier*> list;

    SyntaxModifiers(Pos pos) : SyntaxNode(pos, Kind::Modifiers, nullptr) {}
    void dispose() override;
};

struct SyntaxFile : SyntaxNode {
    List<SyntaxNode*> nodes;

    SyntaxFile(Pos pos) : SyntaxNode(pos, Kind::File, nullptr) {}
    void dispose() override;
    const SourceFile& sourceFile() const;
    const List<SourceToken>& tokens() const;
};

struct SyntaxModule : SyntaxNode {
    SyntaxNode *name;

    SyntaxModule(Pos pos, Node modifiers) : SyntaxNode(pos, Kind::Module, modifiers) {}
    void dispose() override;
};

struct SyntaxImportOrExport : SyntaxNode {
    SyntaxNode *name;
    SyntaxNode *from;
    SyntaxNode *as;

    SyntaxImportOrExport(Pos pos, Kind kind) : SyntaxNode(pos, kind, nullptr) {}
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

    SyntaxCommaList(Pos pos, Node modifiers, List<SyntaxNode*> &nodes) : 
        SyntaxNode(pos, Kind::CommaList, modifiers), nodes(nodes) {}
    void dispose() override;
};

struct SyntaxDotExpression : SyntaxNode {
    SyntaxNode *lhs;
    Pos         dot;
    SyntaxNode *rhs;

    SyntaxDotExpression(Pos pos, Node modifiers, SyntaxNode *lhs, Pos dot, SyntaxNode *rhs) : 
        SyntaxNode(pos, Kind::DotExpression, modifiers), lhs(lhs), dot(dot), rhs(rhs) {}
    void dispose() override;
};

struct SyntaxBoolean : SyntaxNode {
    bool value;

    SyntaxBoolean(Pos pos, Node modifiers, bool value) : 
        SyntaxNode(pos, Kind::Boolean, modifiers), value(value) {}
};

struct SyntaxIdentifier : SyntaxNode {
    Identifier value;
    Keyword    keyword;

    SyntaxIdentifier(Pos pos, Node modifiers, Identifier value, Keyword keyword = Keyword()) : 
        SyntaxNode(pos, Kind::Identifier, modifiers), value(value), keyword(keyword) {}
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