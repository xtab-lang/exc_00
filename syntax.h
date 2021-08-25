#pragma once

namespace exy {
struct SourceFolder;
struct SourceFile;

struct SyntaxTree;
struct SyntaxNode;
struct SyntaxFolder;
struct SyntaxFile;
struct SyntaxModule;

#define DeclareSyntaxNodes(ZM) \
    ZM(Empty)                  \
    /* Modifiers 🡓  */         \
    ZM(Modifier)               \
    ZM(ModifierList)           \
    /* Statements 🡓  */        \
    ZM(Module)                 \
    ZM(Import)                 \
    ZM(Export)                 \
    ZM(Define)                 \
    ZM(ExternBlock)            \
    ZM(Structure)              \
    ZM(Function)               \
    ZM(Block)                  \
    ZM(FlowControl)            \
    ZM(If)                     \
    ZM(Switch)                 \
    ZM(Case)                   \
    ZM(ForIn)                  \
    ZM(For)                    \
    ZM(While)                  \
    ZM(DoWhile)                \
    ZM(Defer)                  \
    ZM(Using)                  \
    /* Variables 🡓  */         \
    ZM(Variable)               \
    /* Binary 🡓  */            \
    ZM(Binary)                 \
    /* Ternary 🡓  */           \
    ZM(Ternary)                \
    ZM(IfExpression)           \
    /* Unary 🡓  */             \
    ZM(UnaryPrefix)            \
    ZM(UnarySuffix)            \
    /* Postfix 🡓  */           \
    ZM(Dot)                    \
    ZM(Call)                   \
    ZM(Index)                  \
    ZM(TypeName)               \
    ZM(Initializer)            \
    /* Enclosed 🡓  */          \
    ZM(Parenthesized)          \
    ZM(Bracketed)              \
    ZM(Angled)                 \
    ZM(Braced)                 \
    /* Interpolation 🡓  */     \
    ZM(Interpolation)          \
    ZM(CodeBlock)              \
    /* Others 🡓  */            \
    ZM(CommaSeparated)         \
    ZM(NameValue)              \
    ZM(Rest)                   \
    ZM(RestParameter)          \
    /* Literals 🡓  */          \
    ZM(Text)                   \
    ZM(Identifier)             \
    ZM(SingleQuoted)           \
    ZM(DoubleQuoted)           \
    ZM(Null)                   \
    ZM(Void)                   \
    ZM(Boolean)                \
    ZM(Number)

#define ZM(zName) struct zName##Syntax;
DeclareSyntaxNodes(ZM)
#undef ZM
//----------------------------------------------------------
enum class SyntaxKind {
    Unknown,
    Folder,
    File,
#define ZM(zName) zName,
    DeclareSyntaxNodes(ZM)
#undef ZM
};
//----------------------------------------------------------
struct SyntaxTree {
    Mem                 mem;
    List<SyntaxFolder*> folders{};
    List<SyntaxModule*> modules{};

    bool initialize();
    void dispose();
private:
    void parse(SyntaxFolder *parent, List<SourceFolder*> &list);
    void parse(SyntaxFolder *parent, List<SourceFile> &list);
    void parse(SyntaxFolder *parent, SourceFolder *folder);
    void parse(SyntaxFolder *parent, SourceFile &file);
    // syntax_modules.cpp
    void discoverModules();
    void printModules();
    void printModule(INT indent, SyntaxModule*);
};
//----------------------------------------------------------
struct SyntaxNode {
    using   Pos = const SourceToken&;
    using OpPos = const SourceToken*;
    using KwPos = const SourceToken*;
    using  OpenPos = const SourceToken*;
    using ClosePos = const SourceToken*;
    using  Kind = SyntaxKind;
    using  Node = SyntaxNode*;
    using Nodes = List<Node>;
    using  Name = IdentifierSyntax*;

    Pos  pos;
    Kind kind;

    SyntaxNode(Pos pos, Kind kind) : pos(pos), kind(kind) {}

    virtual void dispose() {}
    virtual Pos lastPos() const { return pos; }
};
//----------------------------------------------------------
struct SyntaxFolder : SyntaxNode {
    SourceFolder       &src;    // Source.
    SyntaxFolder       *parent; // The immediate parent {SyntaxFolder} of {this} folder.
    List<SyntaxFolder*> folders;// All sub-folders in {this} folder.
    List<SyntaxFile*>   files;  // All files in {this} folder.
    SyntaxFile         *main;   // The 1 and only {SyntaxFile} named 'main.exy' in {this} folder.
    SyntaxFile         *init;   // The 1 and only {SyntaxFile} with same name as {this} folder.

    SyntaxFolder(SourceFolder&, SyntaxFolder *parent);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct SyntaxFile : SyntaxNode {
    SourceFile     &src;             // Source.
    SyntaxFolder   *parent;          // The immediate parent {SyntaxFolder} of {this} file.
    ModuleSyntax   *moduleStatement; // The 1 and only file-scope level {ModuleSyntax} statement in {this} file. May be null.
    Nodes           nodes;           // All the statements in {this} file.

    SyntaxFile(SourceFile&, SyntaxFolder *parent);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct SyntaxModule {
    SourceToken         pos;  // For error reporting.
    List<SyntaxModule*> modules; // All the {SyntaxModule}s sharing the source heirarchy with {this} module.
    List<SyntaxFile*>   files;// All the {SyntaxFile}s contributing to {this} module.
    SyntaxFile         *main; // The 1 and only file named 'main.exy' in {this} module.
    SyntaxFile         *init; // The 1 and {SyntaxFile} with the same name as {this} module.
    Identifier          dotName;
    Identifier          name;
    Identifier          system;

    SyntaxModule(SyntaxFile *firstFile);
    SyntaxModule(SyntaxFile *main, SyntaxFile *init);
    void dispose();
};
//----------------------------------------------------------
// empty := ';'
struct EmptySyntax : SyntaxNode {
    EmptySyntax(Pos pos) : SyntaxNode(pos, Kind::Empty) {}
};
//----------------------------------------------------------
// modifier := 'private' ... 'synchronized'
struct ModifierSyntax : SyntaxNode {
    Keyword value;

    ModifierSyntax(Pos pos);
    auto is(Keyword k) { return value == k ? this : nullptr; }
}; 
//----------------------------------------------------------
// modifier-list := modifier [modifier]+
struct ModifierListSyntax : SyntaxNode {
    List<ModifierSyntax*> nodes;

    ModifierListSyntax(ModifierSyntax *first);
    void dispose() override;
    Pos lastPos() const override;

    ModifierSyntax* contains(Keyword);
};
//----------------------------------------------------------
struct ModuleSyntax : SyntaxNode {
    Node  name;
    KwPos kwAs;
    Name  system;

    ModuleSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct ImportSyntax : SyntaxNode {
    Node  name;
    KwPos kwAs;
    Node  alias;
    KwPos kwFrom;
    Node  source;

    ImportSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// define := 'define' identifier expression
struct DefineSyntax : SyntaxNode {
    Node modifiers;
    Name name;
    Node value;

    DefineSyntax(Node modifiers, Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// externs := [modifiers] 'from' module-name 'import' '{' extern [extern]+ '}'
struct ExternBlockSyntax : SyntaxNode {
    Node     modifiers;
    Node     name;
    OpenPos  open;
    Nodes    nodes;
    ClosePos close;

    ExternBlockSyntax(Pos pos, Node modifiers);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct StructureSyntax : SyntaxNode {
    Node  modifiers;
    Node  name;
    AngledSyntax *parameters;
    Node  attributes;
    OpPos supersOp;
    Node  supers;
    Node  body;

    StructureSyntax(Node modifiers, Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct FunctionSyntax : SyntaxNode {
    Node  modifiers;
    Node  webProtocol; // 'http' ... 'wss'
    Node  httpVerb;    // 'GET' ... 'PATCH'
    Node  name;
    OpPos opName;
    OpPos kwName;
    ParenthesizedSyntax *parameters;
    OpPos fnreturnOp;
    Node  fnreturn;
    OpPos bodyOp;
    Node  body;
    INT awaits, yields, returns;

    FunctionSyntax(Node modifiers, Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct BlockSyntax : SyntaxNode {
    Node     modifiers;
    ParenthesizedSyntax *arguments;
    Nodes    nodes;
    ClosePos close;

    BlockSyntax(Pos pos);
    BlockSyntax(Node modifiers, Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct FlowControlSyntax : SyntaxNode {
    KwPos kwIf;
    Node  expression;
    KwPos kwWith;
    Node  with;

    FlowControlSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct IfSyntax : SyntaxNode {
    Node  modifiers;
    Node  condition;
    Node  iftrue;
    KwPos kwElse;
    Node  ifalse;

    IfSyntax(Node modifiers, Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// switch := 'switch' [expression] case-list
struct SwitchSyntax : SyntaxNode {
    Node condition;
    Node body;
    bool isaTypeSwitch;

    SwitchSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// case := 'case' expression-list statement
//      or 'default' statement
//      or NL '=' expression-list statement
struct CaseSyntax : SyntaxNode {
    Node condition; // May be null (for the 'default' case).
    Node body;

    CaseSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct ForInSyntax : SyntaxNode {
    KwPos kwAwait;
    Node  variables;
    KwPos kwIn;
    Node  expression;
    Node  body;
    KwPos kwElse;
    Node ifnobreak;

    ForInSyntax(Pos pos, Node variables);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct ForSyntax : SyntaxNode {
    Node  initializer;
    Node  condition;
    Node  increment;
    Node  body;
    KwPos kwElse;
    Node  ifnobreak;

    ForSyntax(Pos pos, Node initializer = nullptr);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct WhileSyntax : SyntaxNode {
    Node  condition;
    Node  body;
    KwPos kwElse;
    Node  ifnobreak;

    WhileSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct DoWhileSyntax : SyntaxNode {
    Node  body;
    KwPos kwWhile;
    Node  condition;
    KwPos kwElse;
    Node  ifalse;

    DoWhileSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// defer := 'defer' expression
struct DeferSyntax : SyntaxNode {
    Node expression;

    DeferSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// using := 'using' expression statement
struct UsingSyntax : SyntaxNode {
    Node expression;
    Node statement;

    UsingSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// variable := expression OP expression
struct VariableSyntax : SyntaxNode {
    Node  modifiers;
    Node  name;
    OpPos assign;
    Node  rhs;

    VariableSyntax(Node modifiers, Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// binary := expression OP expression
struct BinarySyntax : SyntaxNode {
    Node lhs;
    Pos  op;
    Node rhs;

    BinarySyntax(Node lhs, Pos op, Node rhs);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct TernarySyntax : SyntaxNode {
    Node  condition;
    Pos   question;
    Node  iftrue;
    OpPos colon;
    Node  ifalse;

    TernarySyntax(Node condition, Pos question);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct IfExpressionSyntax : SyntaxNode {
    Node  iftrue;
    Pos   kwIf;
    Node  condition;
    KwPos kwElse;
    Node  ifalse;

    IfExpressionSyntax(Node iftrue, Pos kwIf);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct UnaryPrefixSyntax : SyntaxNode {
    KwPos kwFrom; // For 'yield' 'from' ...
    Node  expression;
    KwPos kwWith; // For 'delete' expression 'with' ...
    FunctionSyntax *with;

    UnaryPrefixSyntax(Pos op);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct UnarySuffixSyntax : SyntaxNode {
    Node expression;
    Pos  op;

    UnarySuffixSyntax(Node expression, Pos op);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// dot := expr '.' expr
struct DotSyntax : SyntaxNode {
    Node lhs;
    Pos  dot;
    Node rhs;

    DotSyntax(Node lhs, Pos dot, Node rhs = nullptr);
    void dispose() override;
    Pos lastPos() const override;

    void flatten(Nodes &list);
};
//----------------------------------------------------------
template<typename T>
struct PostfixEnclosedSyntax : SyntaxNode {
    Node name;
    T   *arguments;
    PostfixEnclosedSyntax(Node name, T *arguments, Kind kind) : 
        SyntaxNode(name->pos, kind), name(name), arguments(arguments) {}
    void dispose() override {
        name = ndispose(name);
        arguments = ndispose(arguments);
        __super::dispose();
    }
    Pos lastPos() const override {
        if (arguments != nullptr) return arguments->lastPos();
        return name->lastPos();
    }
};
//----------------------------------------------------------
struct CallSyntax : PostfixEnclosedSyntax<ParenthesizedSyntax> {
    KwPos           kwWith;
    FunctionSyntax *with;

    CallSyntax(Node name, ParenthesizedSyntax *arguments);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// index := expr '[' [call-args] ']'
struct IndexSyntax : PostfixEnclosedSyntax<BracketedSyntax> {
    IndexSyntax(Node name, BracketedSyntax *arguments);
};
//----------------------------------------------------------
// typename := identifier '<' [call-args] '>'
struct TypeNameSyntax : PostfixEnclosedSyntax<AngledSyntax> {
    TypeNameSyntax(Node name, AngledSyntax *arguments);
};
//----------------------------------------------------------
// initializer := expr '{' [call-args] '}'
struct InitializerSyntax : PostfixEnclosedSyntax<BracedSyntax> {
    InitializerSyntax(Node name, BracedSyntax *arguments);
};
//----------------------------------------------------------
struct EnclosedSyntax : SyntaxNode {
    Node     value;
    ClosePos close;

    EnclosedSyntax(Pos pos, Kind kind) : SyntaxNode(pos, kind) {}
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// parenthesized-syntax := '(' expression-list ')'
struct ParenthesizedSyntax : EnclosedSyntax {
    ParenthesizedSyntax(Pos pos) : EnclosedSyntax(pos, Kind::Parenthesized) {}
};
//----------------------------------------------------------
// bracketed-syntax := '[' expression-list ']'
struct BracketedSyntax : EnclosedSyntax {
    BracketedSyntax(Pos pos) : EnclosedSyntax(pos, Kind::Bracketed) {}
};
//----------------------------------------------------------
// angled-syntax := '<' expression-list '>'
struct AngledSyntax : EnclosedSyntax {
    AngledSyntax(Pos pos) : EnclosedSyntax(pos, Kind::Angled) {}
};
//----------------------------------------------------------
// braced-syntax := '{' expression-list '}'
struct BracedSyntax : EnclosedSyntax {
    BracedSyntax(Pos pos) : EnclosedSyntax(pos, Kind::Braced) {}
};
//----------------------------------------------------------
struct InterpolationSyntax : SyntaxNode {
    Nodes    nodes;
    ClosePos close;

    InterpolationSyntax(Pos pos, Node first);
    void dispose() override;
    Pos lastPos() const override;
};
struct CodeBlockSyntax : SyntaxNode {
    Node     node;
    ClosePos close;

    CodeBlockSyntax(Pos pos);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// comma-separated := expression [',' expression]+
struct CommaSeparatedSyntax : SyntaxNode {
    Nodes nodes;

    CommaSeparatedSyntax(Node first);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
// name-value := identifier '=' expression
//            or identifier ':' expression
struct NameValueSyntax : SyntaxNode {
    Name name;
    Pos  op;
    Node value;

    NameValueSyntax(Name name, Pos op);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct RestSyntax : SyntaxNode {
    Name name;

    RestSyntax(Pos ellipsis);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct RestParameterSyntax : SyntaxNode {
    Node modifiers;
    Name name;
    Pos  ellipsis;

    RestParameterSyntax(Node modifiers, Pos ellipsis);
    RestParameterSyntax(Node modifiers, Name name, Pos ellipsis);
    void dispose() override;
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct TextSyntax : SyntaxNode {
    String value;
    Pos    end;

    TextSyntax(Pos pos, const String &value, Pos end);
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct IdentifierSyntax : SyntaxNode {
    Identifier value;

    IdentifierSyntax(Pos pos);
};
//----------------------------------------------------------
struct SingleQuotedSyntax : SyntaxNode {
    String value;
    Pos    close;

    SingleQuotedSyntax(Pos pos, const String &value, Pos close);
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct DoubleQuotedSyntax : SyntaxNode {
    String value;
    Pos    close;

    DoubleQuotedSyntax(Pos pos, const String &value, Pos close);
    Pos lastPos() const override;
};
//----------------------------------------------------------
struct NullSyntax : SyntaxNode {
    NullSyntax(Pos pos) : SyntaxNode(pos, Kind::Null) {}
};
//----------------------------------------------------------
struct VoidSyntax : SyntaxNode {
    VoidSyntax(Pos pos) : SyntaxNode(pos, Kind::Void) {}
};
//----------------------------------------------------------
struct BooleanSyntax : SyntaxNode {
    bool value;

    BooleanSyntax(Pos pos);
    BooleanSyntax(Pos pos, bool value);
};
//----------------------------------------------------------
struct NumberSyntax : SyntaxNode {
    union {
        UINT64 u64;
        INT64  i64;
        FLOAT  f32;
        DOUBLE f64;
    };
    Keyword type;

    NumberSyntax(Pos pos);
};
} // namespace exy
