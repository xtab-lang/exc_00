#pragma once

namespace exy {

#define DeclarePunctuationTokens(ZM)        \
    ZM(Unknown,                 "UNKNOWN")  \
    ZM(EndOfFile,               "EOF")      \
    ZM(Space,                   "SP")       \
    ZM(NewLine,                 "NL")       \
    ZM(Comma,                   ",")        \
    ZM(SemiColon,               ";")        \
    ZM(BackSlash,               "\\")       \
    ZM(SingleQuote,             "'")        \
    ZM(DoubleQuote,             "\"")       \
    ZM(WideSingleQuote,         "w'")       \
    ZM(WideDoubleQuote,         "w\"")      \
    ZM(RawSingleQuote,          "r'")       \
    ZM(RawDoubleQuote,          "r\"")      \
    ZM(Hash,                    "#")        \
    ZM(HashHash,                "##")       \
    ZM(At,                      "@")        \
    ZM(AtAt,                    "@@")       \
    ZM(Dot,                     ".")        \
    ZM(DotDot,                  "..")       \
    ZM(Ellipsis,                "...")      \
    ZM(Colon,                   ":")        \
    ZM(ColonColon,              "::")       \
    ZM(DashArrow,               "->")       \
    ZM(AssignArrow,             "=>")       \
    ZM(OpenSingleLineComment,   "//")       \
    ZM(OpenMultiLineComment,    "/*")       \
    ZM(CloseMultiLineComment,   "*/")

#define DeclareGroupingTokens(ZM)   \
    ZM(OpenCurly,           "{")    \
    ZM(CloseCurly,          "}")    \
    ZM(HashOpenParen,       "#(")   \
    ZM(OpenParen,           "(")    \
    ZM(CloseParen,          ")")    \
    ZM(OpenCloseParen,      "()")   \
    ZM(HashOpenBracket,     "#[")   \
    ZM(OpenBracket,         "[")    \
    ZM(CloseBracket,        "]")    \
    ZM(OpenCloseBracket,    "[]")   \
    ZM(OpenAngle,           "<")    \
    ZM(CloseAngle,          ">")    \
    ZM(HashOpenCurly,       "#{")   \
    ZM(CloseCurlyHash,      "}#")

#define DeclareOperatorTokens(ZM)\
/* Binary operators ↓ */        \
    /* Compound assignment ↓ */ \
    ZM(OrAssign,        "|=")   \
    ZM(XOrAssign,       "^=")   \
    ZM(AndAssign,       "&=")   \
    ZM(LeftShiftAssign, "<<=")  \
    ZM(RightShiftAssign,">>=")  \
    ZM(UnsignedRightShiftAssign,">>>=")\
    ZM(MinusAssign,     "-=")   \
    ZM(PlusAssign,      "+=")   \
    ZM(MultiplyAssign,  "*=")   \
    ZM(DivideAssign,    "/=")   \
    ZM(RemainderAssign, "%=")   \
    ZM(DivRemAssign,    "%%=")  \
    ZM(BitwiseNotAssign,"~=")   \
    ZM(ExponentiationAssign,  "**=")   \
    /* Assignment */            \
    ZM(Assign,          "=")    \
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
    ZM(LessOrEqual,     "<=")   \
    ZM(Greater,         ">")    \
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
/* Ternary operators ↓ */       \
    ZM(Question,        "?")    \
/* Variable declaration operator ↓ */\
    ZM(ColonAssign,     ":=")   \
/* Unary operators ↓ */         \
    /* Prefix ↓ */              \
    ZM(UnaryMinus,      "-")    \
    ZM(UnaryPlus,       "+")    \
    ZM(Dereference,     "*")    \
    ZM(AddressOf,       "&")    \
    ZM(LogicalNot,      "!")    \
    ZM(BitwiseNot,      "~")    \
    /* Prefix and suffix ↓ */   \
    ZM(MinusMinus,      "--")   \
    ZM(PlusPlus,        "++")   \
    /* Suffix ↓ */              \
    ZM(Pointer,          "*")   \
    ZM(Reference,       "&")

#define DeclareTextTokens(ZM) \
    ZM(Text,             "")   \
    ZM(SingleLineComment,"")   \
    ZM(MultiLineComment, "")   \
    ZM(Decimal,          "")   \
    ZM(Hexadecimal,      "")   \
    ZM(Binary,           "")   \
    ZM(Octal,            "")   \
    ZM(Float,            "")   \
    ZM(DecimalFloat,     "")   \
    ZM(HexadecimalFloat, "")   \
    ZM(BinaryFloat,      "")   \
    ZM(OctalFloat,       "")

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

} // namespace exy