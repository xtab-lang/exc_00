//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-22
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef COMPILER_H_
#define COMPILER_H_

namespace exy {
namespace comp_pass {
void run(int compId);
} // namespace comp_pass

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
    ZM(Struct,       "struct")      \
    ZM(Object,       "object")      \
    ZM(Union,        "union")       \
    ZM(Enum,         "enum")        \
    ZM(Lambda,       "lambda")      \
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
    ZM(Module,      "module")       \
    ZM(Import,      "import")       \
    ZM(Export,      "export")       \
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
    /* Alias */                     \
    ZM(Let,         "let")          \
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
    ZM(True,        "true")         \
    ZM(False,       "false")        \
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

enum class Keyword {
    None,
#define ZM(zName, zSize) zName,
    DeclareKeywords(ZM)
#undef ZM
    _begin_modifiers,
#define ZM(zName, zSize) zName,
    DeclareModifiers(ZM)
#undef ZM
    _end_modifiers,
    _begin_udts,
#define ZM(zName, zText) zName,
    DeclareUserDefinedTypeKeywords(ZM)
#undef ZM
    _end_utds,
    _begin_builtins,
#define ZM(zName, zText) zName,
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

enum class Tok : BYTE {
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
//----End forward declarations

//------------------------------------------------------------------------------------------------
struct SourceChar {
    const char *pos;
    int         line;
    int         col;

    SourceChar() = delete;
    SourceChar(const char *pos, int line, int col) : pos(pos), line(line), col(col) {}
};

struct SourceRange {
    SourceChar start;
    SourceChar end;

    SourceRange() = delete;
    SourceRange(const SourceChar &start, const SourceChar &end) : start(start), end(end) {}

    String value() const { return { start.pos, (int)(end.pos - start.pos), 0u }; }
};

struct SourceToken {
    const SourceFile *file;
    SourceRange       range;
    Tok               kind;
    Keyword           keyword;

    SourceToken() = delete;
    SourceToken(const SourceToken&) = default;
    SourceToken(const SourceFile &file, const SourceChar &start, const SourceChar &end, Tok kind) :
        file(&file), range(start, end), kind(kind) {}
    String name() const;
    String value() const;
    String sourceValue() const { return range.value(); }
};

//------------------------------------------------------------------------------------------------
struct Compiler {
    SourceTree *source{};
    SyntaxTree *syntax{};
    int         errors{};
    int         warnings{};
    int         infos{};
    struct {
        String path{};
        int    maxFileSize{};
        int    defaultFilesPerThread{};
    } options{};

    void error(const SourceToken*, const char*, const char*, int, const char *fmt, ...);
    void error(const SourceToken&, const char*, const char*, int, const char *fmt, ...);
private:
    void run();

    enum class HighlightKind {
        Error,
        Warning,
        Info
    };
    void highlight(HighlightKind, const SourceToken&, const SourceToken&, const char*, const char*, int, const char*, va_list);
    void printMessage(HighlightKind, const SourceFile&, const SourceRange&, const char*, va_list);
    void printCppLocation(const char *cppFile, const char *cppFunc, int cppLine);

    struct Line {
        const char *start;
        const char *end;
        int   number;
    };
    static const int maxLines       = 6;
    static const int maxLinesAbove  = 2;
    static const int lineNumberSize = 6;
    void printLines(HighlightKind, const String &source, const SourceRange&);
    void collectLines(const String &source, const SourceRange&, Line*);
    void highlightLines(HighlightKind, const SourceRange&, Line*);
    void highlightLine(HighlightKind, const SourceRange&, const Line&);

    friend void comp_pass::run(int);
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

#define err(token, msg, ...) comp.error(token, __FILE__, __FUNCTION__, __LINE__, msg, __VA_ARGS__)
} // namespace exy

#endif // COMPILER_H_