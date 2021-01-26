//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-22
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef COMPILER_H_
#define COMPILER_H_

namespace exy {
namespace compiler {
void run(int compId);
} // namespace compiler

#define DeclareBuiltinTypeKeywords(ZM)   \
/* Type declaration keywords. */         \
    /* Non-SIMD types. */                \
    ZM(Void,        0)   \
    ZM(Char,        1)   \
    ZM(Int8,        1)   \
    ZM(Int16,       2)   \
    ZM(Int32,       4)   \
    ZM(Int64,       8)   \
    ZM(Bool,        1)   \
    ZM(UInt8,       1)   \
    ZM(WChar,       2)   \
    ZM(UInt16,      2)   \
    ZM(Utf8,        4)   \
    ZM(UInt32,      4)   \
    ZM(UInt64,      8)   \
    ZM(Float,       4)   \
    ZM(Double,      8)   \
    /* SIMD types (16 B; AVX128). */     \
    ZM(Floatx4,     16)  \
    ZM(Doublex2,    16)  \
    ZM(Int8x16,     16)  \
    ZM(Int16x8,     16)  \
    ZM(Int32x4,     16)  \
    ZM(Int64x2,     16)  \
    ZM(UInt8x16,    16)  \
    ZM(UInt16x8,    16)  \
    ZM(UInt32x4,    16)  \
    ZM(UInt64x2,    16)  \
    /* SIMD types (32 B; AVX256). */     \
    ZM(Floatx8,     32)  \
    ZM(Doublex4,    32)  \
    ZM(Int8x32,     32)  \
    ZM(Int16x16,    32)  \
    ZM(Int32x8,     32)  \
    ZM(Int64x4,     32)  \
    ZM(UInt8x32,    32)  \
    ZM(UInt16x16,   32)  \
    ZM(UInt32x8,    32)  \
    ZM(UInt64x4,    32)  \
    /* SIMD types (64 B; AVX512). */     \
    ZM(Floatx16,    64)  \
    ZM(Doublex8,    64)  \
    ZM(Int8x64,     64)  \
    ZM(Int16x32,    64)  \
    ZM(Int32x16,    64)  \
    ZM(Int64x8,     64)  \
    ZM(UInt8x64,    64)  \
    ZM(UInt16x32,   64)  \
    ZM(UInt32x16,   64)  \
    ZM(UInt64x8,    64)

#define DeclareUserDefinedTypeKeywords(ZM) \
    /* User-Defined Types (UDTs) */ \
    ZM(Module,       "module")      \
    ZM(Struct,       "struct")      \
    ZM(Object,       "object")      \
    ZM(Union,        "union")       \
    ZM(Enum,         "enum")        \
    ZM(Lambda,       "lambda")      \
    /* Function Types */            \
    ZM(Extern,       "extern")      \
    ZM(Fn,           "fn")          \
    ZM(UrlHandler,   "urlhandler")  \
    ZM(Html,         "html")        \
    ZM(Css,          "css")         \
    ZM(Js,           "js")          \
    ZM(Json,         "json")        \
    ZM(Sql,          "sql")         \
    ZM(Blob,         "blob")        \

#define DeclareKeywords(ZM)         \
    /* Namespace usage */           \
    ZM(Import,      "import")       \
    ZM(Export,      "export")       \
    /* Alias */                     \
    ZM(Let,         "let")          \
    /* Control */                   \
    ZM(If,          "if")           \
    ZM(Else,        "else")         \
    ZM(Switch,      "switch")       \
    ZM(Case,        "case")         \
    ZM(Default,     "default")      \
    /* Flow break */                \
    ZM(Break,       "break")        \
    ZM(Continue,    "continue")     \
    ZM(Return,      "return")       \
    /* Generator */                 \
    ZM(Yield,       "yield")        \
    ZM(Awaut,       "await")        \
    /* Loop */                      \
    ZM(For,         "for")          \
    ZM(In,          "in")           \
    /* Binary functions */          \
    ZM(As,          "as")           \
    ZM(To,          "to")           \
    ZM(Is,          "is")           \
    ZM(NotIs,       "!is")          \
    ZM(NotIn,       "!in")          \
    /* Unary functions */           \
    ZM(AlignOf,     "alignof")      \
    ZM(SizeOf,      "sizeof")       \
    ZM(TypeOf,      "typeof")       \
    ZM(NameOf,      "nameof")       \
    ZM(ValueOf,     "valueof")      \
    ZM(New,         "new")          \
    ZM(Delete,      "delete")       \
    /* Literals */                  \
    ZM(Null,        "null")         \
    ZM(void_,       "void")         \
    ZM(False,       "false")        \
    ZM(True,        "true")         \
    /* Variables */                 \
    ZM(This,        "this")         \
    ZM(Super,       "super")        \
    /* Others */                    \
    ZM(Defer,       "defer")        \
    ZM(From,        "from")         \
    ZM(With,        "with")

#define DeclareModifiers(ZM)        \
    ZM(Private,     "private")      \
    ZM(Static,      "static")       \
    ZM(Const,       "const")        \
    ZM(ReadOnly,    "readonly")     \
    ZM(Auto,        "auto")         \
    ZM(Var,         "var")          \
    ZM(Async,       "async")        \
    ZM(Abstract,    "abstract")     \
    ZM(Override,    "override")     \
    ZM(Synchronized,"synchronized")

enum class Keyword {
    None,
#define ZM(zName, zText) zName,
    DeclareKeywords(ZM)
#undef ZM
    _begin_modifiers,
#define ZM(zName, zText) zName,
    DeclareModifiers(ZM)
#undef ZM
    _end_modifiers,
    _begin_udts,
#define ZM(zName, zText) zName,
    DeclareUserDefinedTypeKeywords(ZM)
#undef ZM
    _end_utds,
    _begin_builtins,
#define ZM(zName, zSize) zName,
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
    _end_builtins
};

#define DeclarePunctuationTokens(ZM)        \
    ZM(Unknown,                 "UNKNOWN")  \
    ZM(EndOfFile,               "EOF")      \
    ZM(Space,                   "SP")       \
    ZM(NewLine,                 "NL")       \
    ZM(Hash,                    "#")        \
    ZM(HashHash,                "##")       \
    ZM(At,                      "@")        \
    ZM(AtAt,                    "@@")       \
    ZM(Ellipsis,                "...")      \
    ZM(DotDot,                  "..")       \
    ZM(Dot,                     ".")        \
    ZM(Comma,                   ",")        \
    ZM(Colon,                   ":")        \
    ZM(ColonColon,              "::")       \
    ZM(SemiColon,               ";")        \
    ZM(BackSlash,               "\\")       \
    ZM(SingleQuote,             "'")        \
    ZM(DoubleQuote,             "\"")       \
    ZM(WideSingleQuote,         "w'")       \
    ZM(WideDoubleQuote,         "w\"")      \
    ZM(RawSingleQuote,          "r'")       \
    ZM(RawDoubleQuote,          "r\"")      \
    ZM(OpenSingleLineComment,   "//")       \
    ZM(OpenMultiLineComment,    "/*")       \
    ZM(CloseMultiLineComment,   "*/")

#define DeclareGroupingTokens(ZM)   \
    ZM(OpenCurly,           "{")    \
    ZM(CloseCurly,          "}")    \
    ZM(HashOpenParen,       "#(")   \
    ZM(OpenParen,           "(")    \
    ZM(CloseParen,          ")")    \
    ZM(HashOpenBracket,     "#[")   \
    ZM(OpenBracket,         "[")    \
    ZM(CloseBracket,        "]")    \
    ZM(OpenAngle,           "<")    \
    ZM(CloseAngle,          ">")    \
    ZM(HashOpenCurly,       "#{")   \
    ZM(CloseCurlyHash,      "}#")

#define DeclareOperatorTokens(ZM)    \
/* Binary operators ↓ */        \
    /* Assignment ↓ */          \
    ZM(OrAssign,        "|=")   \
    ZM(XOrAssign,       "^=")   \
    ZM(AndAssign,       "&=")   \
    ZM(LeftShiftAssign, "<<=")  \
    ZM(RightShiftAssign,">>=")  \
    ZM(UnsignedRightShiftAssign,">>>=")\
    ZM(MinusAssign,     "-=")   \
    ZM(PlusAssign,      "+=")   \
    ZM(MultiplyAssign,  "*=")   \
    ZM(ExponentiationAssign,  "**=")   \
    ZM(DivideAssign,    "/=")   \
    ZM(RemainderAssign, "%=")   \
    ZM(DivRemAssign,    "%%=")  \
    ZM(BitwiseNotAssign,"~=")   \
    /* Assignment */            \
    ZM(Assign,          "=")    \
    /* Ternary ↓ */             \
    ZM(Question,        "?")    \
    /* Logical ↓ */             \
    ZM(OrOr,            "||")   \
    ZM(AndAnd,          "&&")   \
    ZM(QuestionQuestion,"??")   \
    /* Bitwise ↓ */             \
    ZM(Or,              "|")    \
    ZM(XOr,             "^")    \
    ZM(And,             "&")    \
    /* Relational ↓ */          \
    ZM(NotEquivalent,   "!==")  \
    ZM(NotEqual,        "!=")   \
    ZM(Equivalent,      "===")  \
    ZM(Equal,           "==")   \
    ZM(Less,            "<")    \
    ZM(LessOrEqual,     ">=")   \
    ZM(Greater,         "<")    \
    ZM(GreaterOrEqual,  ">=")   \
    /* Shift ↓ */               \
    ZM(LeftShift,       "<<")   \
    ZM(RightShift,      ">>")   \
    ZM(UnsignedRightShift,">>>")\
    /* Additive ↓ */            \
    ZM(Minus,           "-")    \
    ZM(Plus,            "+")    \
    /* Multiplicative ↓ */      \
    ZM(Multiply,        "*")    \
    ZM(Divide,          "/")    \
    ZM(Remainder,       "%")    \
    ZM(DivRem,          "%%")   \
    ZM(Exponentiation,  "**")   \
/* Unary operators ↓ */         \
    /* Prefix ↓ */              \
    ZM(UnaryMinus,      "-")    \
    ZM(UnaryPlus,       "+")    \
    ZM(Dereference,     "*")    \
    ZM(AddressOf,       "&")    \
    ZM(LogicalNot,      "!")    \
    ZM(BitwiseNot,      "~")    \
    /* Suffix ↓ */              \
    ZM(Pointer,         "*")    \
    ZM(Reference,       "&")    \
    /* Prefix and suffix ↓ */   \
    ZM(MinusMinus,      "--")   \
    ZM(PlusPlus,        "++")

#define DeclareTextTokens(ZM) \
    ZM(Text,            "")   \
    ZM(Decimal,         "")   \
    ZM(Hexadecimal,     "")   \
    ZM(Binary,          "")   \
    ZM(Octal,           "")   \
    ZM(Float,           "")

enum class Tok {
#define ZM(zName, zText) zName,
    DeclarePunctuationTokens(ZM)
#undef ZM
#define ZM(zName, zText) zName,
    DeclareGroupingTokens(ZM)
#undef ZM
#define ZM(zName, zText) zName,
    DeclareOperatorTokens(ZM)
#undef ZM
#define ZM(zName, zText) zName,
    DeclareTextTokens(ZM)
#undef ZM
};
//--Begin forward declarations
struct SourceChar;
struct SourceRange;
struct SourceToken;
struct SourceFile;
struct SourceTree;

struct SyntaxTree;
struct SyntaxNode;

struct AstTree;
struct AstNode;

struct IrTree;
struct IrNode;

struct PeTree;

namespace stx2ast_pass {
struct Typer;
}
//----End forward declarations

//------------------------------------------------------------------------------------------------
struct SourceChar {
    const char *pos;
    int         line;
    int         col;

    SourceChar() = delete;
    SourceChar(const char *pos, int line, int col) : pos(pos), line(line), col(col) {}
    bool operator>(const SourceChar &other) const { return line > other.line || (line == other.line && col > other.col); }
    bool operator>=(const SourceChar &other) const { return line > other.line || (line == other.line && col >= other.col); }
    bool operator<(const SourceChar &other) const { return line < other.line || (line == other.line && col < other.col); }
    bool operator<=(const SourceChar &other) const { return line < other.line || (line == other.line && col <= other.col); }
};

struct SourceRange {
    SourceChar start;
    SourceChar end;

    SourceRange() = delete;
    SourceRange(const SourceRange&) = default;
    SourceRange(const SourceChar &start, const SourceChar &end) : start(start), end(end) {
        Assert(start <= end);
    }
    bool operator>(const SourceRange &other) const { return start > other.end; }
    bool operator>=(const SourceRange &other) const { return start >= other.end; }
    bool operator<(const SourceRange &other) const { return end < other.start; }
    bool operator<=(const SourceRange &other) const { return end <= other.start; }
    String value() const { return { start.pos, (int)(end.pos - start.pos), 0u }; }
};

struct SourceLocation {
    const SourceFile &file;
    SourceRange       range;

    SourceLocation() = delete;
    SourceLocation(const SourceLocation&) = default;
    SourceLocation(const SourceFile &file, const SourceChar &start, const SourceChar &end) 
        : file(file), range(start, end) {}
    String sourceValue() const { return range.value(); }

    SourceLocation& operator=(const SourceLocation &other);
};

using Loc = const SourceLocation&;

struct SourceToken {
    SourceLocation loc;
    Tok            kind;
    Keyword        keyword;

    SourceToken() = delete;
    SourceToken(const SourceToken&) = default;
    SourceToken(const SourceFile &file, const SourceChar &start, const SourceChar &end, Tok kind) :
        loc(file, start, end), kind(kind) {}
    String name() const;
    String value() const;
    static String name(Tok);
    static String name(Keyword);
    static String value(Tok);
    static String value(Keyword);
    String sourceValue() const { return loc.sourceValue(); }

    SourceToken& operator=(const SourceToken&) = default;
};
//------------------------------------------------------------------------------------------------
struct Compiler {
    SourceTree *source{};
    SyntaxTree *syntax{};
    AstTree    *ast{};
    IrTree     *ir{};
    stx2ast_pass::Typer *typer{};
    PeTree     *pe;
    int         errors{};
    int         warnings{};
    int         infos{};
    struct {
        struct {
            String sourceFolder;
            String outputFolder;
        } path;
        int    maxFileSize;
        int    defaultFilesPerThread;
        int    defaultModulesPerThread;
        int    defaultSymbolsPerThread;
        struct {
            int maxScopeDepth;
        } typer;
    } options;

    void error(const char*, const SourceToken*, const char*, const char*, int, const char*, ...);
    void error(const char*, const SourceToken&, const char*, const char*, int, const char*, ...);
    void error(const char*, const SourceLocation&, const char*, const char*, int, const char*, ...);

    void error(const char*, const SyntaxNode*, const char*, const char*, int, const char*, ...);
    void error(const char*, const AstNode*, const char*, const char*, int, const char*, ...);
    void error(const char*, const IrNode*, const char*, const char*, int, const char*, ...);
private:
    void compile();
    void runBinaries();

    enum class HighlightKind {
        Error,
        Warning,
        Info
    };
    void highlight(HighlightKind, const char*, const SourceLocation&, const char*, const char*, int, const char*, va_list);
    void printMessage(HighlightKind, const char*, const SourceLocation&, const char*, va_list);
    void printCppLocation(const char *cppFile, const char *cppFunc, int cppLine);

    struct Line {
        const char *start;
        const char *end;
        int   number;
    };
    static const int maxLines       = 6;
    static const int maxLinesAbove  = 2;
    static const int lineNumberSize = 6;
    void printLines(HighlightKind, const SourceLocation&);
    void collectLines(const String &source, const SourceRange&, Line*);
    void highlightLines(HighlightKind, const SourceRange&, Line*);
    void highlightLine(HighlightKind, const SourceRange&, const Line&);

    friend void compiler::run(int);
    friend static Line makeLineFrom(const String&, const char*, int);
    friend static Line makePreviousLine(const String&, const char*, int);
    friend static Line makeNextLine(const String&, const char*, int);
};

__declspec(selectany) Compiler comp;

template<typename T>
T* ndispose(T *node) {
    if (node) {
        node->dispose();
    }
    return nullptr;
}

template<typename T>
void ldispose(List<T*> &list) {
    list.dispose([](T *x) { ndispose(x); });
}

template<typename T>
void ldispose(Queue<T> &queue) {
    queue.dispose([](T *x) { ndispose(x); });
}

template<typename T>
void ldispose(Dict<T*> &dict) {
    dict.dispose([](T *x) { ndispose(x); });
}

#define print_error(pass, token, msg, ...) comp.error((pass), (token), __FILE__, __FUNCTION__, __LINE__, (msg), __VA_ARGS__)

int signedSize(INT64 n);
int unsignedSize(UINT64 n);

struct Status {
    enum Value { None, Busy, Done };
    Value value;

    bool isIdle() const { return value == None; }
    bool isBusy() const { return value == Busy; }
    bool isDone() const { return value == Done; }
    bool isNotDone() const { return value != Done; }
    bool isNotBusy() const { return value != Busy; }

    void begin() { Assert(isIdle()); value = Busy; }
    void end()   { Assert(isBusy()); value = Done; }
};

} // namespace exy

#endif // COMPILER_H_