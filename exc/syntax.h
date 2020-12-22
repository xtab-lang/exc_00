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
    struct SyntaxLet;
    struct SyntaxCommaList;
    struct SyntaxUnary;
        struct SyntaxUnaryPrefix;
        struct SyntaxUnarySuffix;
    struct SyntaxDotExpression;
    struct SyntaxArgumentizedExpression;
        struct SyntaxCallExpression;
        struct SyntaxIndexExpression;
        struct SyntaxTypeNameExpression;
        struct SyntaxObjectExpression;
    struct SyntaxNameValue;
    struct SyntaxLiteral;
        struct SyntaxNull;
        struct SyntaxVoid;
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
    ZM(Let)                      \
    ZM(CommaList)                \
    ZM(UnaryPrefix)              \
    ZM(UnarySuffix)              \
    ZM(DotExpression)            \
    ZM(CallExpression)           \
    ZM(IndexExpression)          \
    ZM(TypeNameExpression)       \
    ZM(ObjectExpression)         \
    ZM(NameValue)                \
    ZM(Literal)                  \
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
    using  Kind = SyntaxKind;
    using  Node = SyntaxNode*;
    using Nodes = List<Node>;

    Pos  pos;
    Node modifiers;
    Kind kind;

    SyntaxNode(Pos pos, Kind kind, Node modifiers) : pos(pos), modifiers(modifiers), kind(kind) {}
    virtual void dispose();
    virtual Pos lastpos() const { return pos; }
    String kindName() const;
    static String kindName(Kind);
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
    List<SyntaxModifier*> nodes;

    SyntaxModifiers(Pos pos) : SyntaxNode(pos, Kind::Modifiers, nullptr) {}
    void dispose() override;
    Pos lastpos() const override;
    bool contains(Keyword) const;
};

struct SyntaxFile : SyntaxNode {
    SyntaxModule *mod;
    Nodes         nodes;

    SyntaxFile(Pos pos) : SyntaxNode(pos, Kind::File, nullptr) {}
    void dispose() override;
    Pos lastpos() const override;
    const SourceFile& sourceFile() const;
    const List<SourceToken>& tokens() const;
};

struct SyntaxModule : SyntaxNode {
    Node name;

    SyntaxModule(Pos pos, Node modifiers) : SyntaxNode(pos, Kind::Module, modifiers) {}
    void dispose() override;
    Pos lastpos() const override;
};

struct SyntaxImportOrExport : SyntaxNode {
    Node name;
    Node from;
    Node as;

    SyntaxImportOrExport(Pos pos, Kind kind) : SyntaxNode(pos, kind, nullptr) {}
    void dispose() override;
    Pos lastpos() const override;
};

struct SyntaxImport : SyntaxImportOrExport {
    SyntaxImport(Pos pos) : SyntaxImportOrExport(pos, Kind::Import) {}
};

struct SyntaxExport : SyntaxImportOrExport {
    SyntaxExport(Pos pos) : SyntaxImportOrExport(pos, Kind::Export) {}
};

struct SyntaxLet : SyntaxNode {
    Node name;
    Node value;

    SyntaxLet(Pos pos, Node modifiers) : SyntaxNode(pos, Kind::Let, modifiers) {}
    void dispose() override;
    Pos lastpos() const override;
};

struct SyntaxCommaList : SyntaxNode {
    const SourceToken *close;
    Nodes              nodes;

    SyntaxCommaList(Pos pos, Node modifiers) : 
        SyntaxNode(pos, Kind::CommaList, modifiers) {}
    SyntaxCommaList(Pos pos, Node modifiers, Nodes &nodes) : 
        SyntaxNode(pos, Kind::CommaList, modifiers), nodes(nodes) {}
    SyntaxCommaList(Pos pos, Node modifiers, Nodes &nodes, Pos close) : 
        SyntaxNode(pos, Kind::CommaList, modifiers), nodes(nodes), close(&close) {}
    void dispose() override;
    Pos lastpos() const override;
};

struct SyntaxUnary : SyntaxNode {
    Pos  op;
    Node value;

    SyntaxUnary(Pos pos, Node modifiers, Kind kind, Node value, Pos op) : 
        SyntaxNode(pos, kind, modifiers), op(op), value(value) {}
    void dispose() override;
};

struct SyntaxUnaryPrefix : SyntaxUnary {
    SyntaxUnaryPrefix(Pos pos, Node modifiers, Pos op) : 
        SyntaxUnary(pos, modifiers, Kind::UnaryPrefix, nullptr, op) {}
    Pos lastpos() const override;
};

struct SyntaxUnarySuffix : SyntaxUnary {
    SyntaxUnarySuffix(Pos pos, Node modifiers, Node value, Pos op) : 
        SyntaxUnary(pos, modifiers, Kind::UnarySuffix, value, op) {}
    Pos lastpos() const override;
};

struct SyntaxDotExpression : SyntaxNode {
    Node lhs;
    Pos  dot;
    Node rhs;

    SyntaxDotExpression(Pos pos, Node modifiers, Node lhs, Pos dot, Node rhs) : 
        SyntaxNode(pos, Kind::DotExpression, modifiers), lhs(lhs), dot(dot), rhs(rhs) {}
    SyntaxDotExpression(Pos pos, Node modifiers, Node lhs, Pos dot) : 
        SyntaxNode(pos, Kind::DotExpression, modifiers), lhs(lhs), dot(dot) {}
    void dispose() override;
    Pos lastpos() const override;
};

struct SyntaxArgumentizedExpression : SyntaxNode {
    Node             name;
    SyntaxCommaList *arguments;

    SyntaxArgumentizedExpression(Pos pos, Kind kind, Node modifiers, Node name) : 
        SyntaxNode(pos, kind, modifiers), name(name) {}
    void dispose() override;
    Pos lastpos() const override;
};

struct SyntaxCallExpression : SyntaxArgumentizedExpression {
    SyntaxCallExpression(Pos pos, Node modifiers, Node name) : 
        SyntaxArgumentizedExpression(pos, Kind::CallExpression, modifiers, name) {}
};

struct SyntaxIndexExpression : SyntaxArgumentizedExpression {
    SyntaxIndexExpression(Pos pos, Node modifiers, Node name) : 
        SyntaxArgumentizedExpression(pos, Kind::IndexExpression, modifiers, name) {}
};

struct SyntaxTypeNameExpression : SyntaxArgumentizedExpression {
    SyntaxTypeNameExpression(Pos pos, Node modifiers, Node name) : 
        SyntaxArgumentizedExpression(pos, Kind::TypeNameExpression, modifiers, name) {}
};

struct SyntaxObjectExpression : SyntaxArgumentizedExpression {
    SyntaxObjectExpression(Pos pos, Node modifiers, Node name) : 
        SyntaxArgumentizedExpression(pos, Kind::ObjectExpression, modifiers, name) {}
};

struct SyntaxNameValue : SyntaxNode {
    Node               name;
    Node               typeName;
    const SourceToken *assign;
    Node               value;

    SyntaxNameValue(Pos pos, Node modifiers, Node name) : 
        SyntaxNode(pos, Kind::NameValue, modifiers), name(name) {}
    void dispose() override;
    Pos lastpos() const override;
};

#define DeclareSyntaxLiteralKinds(ZM) \
    ZM(Void)  ZM(Null) \
    ZM(Char)  ZM(Bool)   ZM(WChar)  ZM(Utf8)   \
    ZM(Int8)  ZM(Int16)  ZM(Int32)  ZM(Int64)  \
    ZM(UInt8) ZM(UInt16) ZM(UInt32) ZM(UInt64) \
    ZM(Float) ZM(Double)

enum class SyntaxLiteralKind {
    None,
#define ZM(zName) zName,
    DeclareSyntaxLiteralKinds(ZM)
#undef ZM
};

struct SyntaxLiteral : SyntaxNode {
    union {
        char   ch;
        bool   b;
        wchar_t wch;
        BYTE   utf8[4];
        INT8   i8;
        UINT8  u8;
        INT32  i16;
        UINT16 u16;
        INT32  i32;
        UINT32 u32;
        INT64  i64;
        UINT64 u64;
        float  f32;
        double f64;
    };
    SyntaxLiteralKind literalKind;

    SyntaxLiteral(Pos pos, Node modifiers, SyntaxLiteralKind literalKind, UINT64 value) : 
        SyntaxNode(pos, Kind::Literal, modifiers), u64(value), literalKind(literalKind) {}
};

struct SyntaxVoid : SyntaxLiteral {
    SyntaxVoid(Pos pos, Node modifiers) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::Void, 0ui64) {}
};

struct SyntaxNull : SyntaxLiteral {
    SyntaxNull(Pos pos, Node modifiers) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::Null, 0ui64) {}
};

struct SyntaxBoolean : SyntaxLiteral {
    SyntaxBoolean(Pos pos, Node modifiers, bool value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::Bool, INT64(value)) {}
};

struct SyntaxInt8 : SyntaxLiteral {
    SyntaxInt8(Pos pos, Node modifiers, INT8 value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::Int8, INT64(value)) {}
};

struct SyntaxInt16 : SyntaxLiteral {
    SyntaxInt16(Pos pos, Node modifiers, INT16 value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::Int16, INT64(value)) {}
};

struct SyntaxInt32 : SyntaxLiteral {
    SyntaxInt32(Pos pos, Node modifiers, INT32 value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::Int32, INT64(value)) {}
};

struct SyntaxInt64 : SyntaxLiteral {
    SyntaxInt64(Pos pos, Node modifiers, INT64 value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::Int64, value) {}
};

struct SyntaxUInt8 : SyntaxLiteral {
    SyntaxUInt8(Pos pos, Node modifiers, UINT8 value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::UInt8, UINT64(value)) {}
};

struct SyntaxUInt16 : SyntaxLiteral {
    SyntaxUInt16(Pos pos, Node modifiers, UINT16 value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::UInt16, UINT64(value)) {}
};

struct SyntaxUInt32 : SyntaxLiteral {
    SyntaxUInt32(Pos pos, Node modifiers, UINT32 value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::UInt32, UINT64(value)) {}
};

struct SyntaxUInt64 : SyntaxLiteral {
    SyntaxUInt64(Pos pos, Node modifiers, UINT64 value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::UInt64, value) {}
};

struct SyntaxFloat: SyntaxLiteral {
    SyntaxFloat(Pos pos, Node modifiers, float value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::Float,
                      UINT64(meta::reinterpret<UINT32>(value))) {}
};

struct SyntaxDouble : SyntaxLiteral {
    SyntaxDouble(Pos pos, Node modifiers, double value) : 
        SyntaxLiteral(pos, modifiers, SyntaxLiteralKind::Double, meta::reinterpret<UINT64>(value)) {}
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
} // namespace exy

#endif // SYNTAX_H