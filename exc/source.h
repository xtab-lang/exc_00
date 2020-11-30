//////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-25
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

#pragma once
#ifndef SOURCE_H_
#define SOURCE_H_

namespace exy {
enum TokenKind {
    Unknown,
    EndOfFile,  // EOF
    Space,      // SP+
    NewLine,    // SP* NL NewLine

    /* Punctuation */
    BackSlash,      // '\'
    Comma,          // ','

    /* Grouping operators */
    OpenParen,      // '('
    OpenBracket,    // '['
    OpenCurly,      // '{'
    OpenAngle,      // '<'
    HashOpenCurly,  // '#{'
    CloseParen,     // ')'
    CloseBracket,   // ']'
    CloseCurly,     // '}'
    CloseAngle,     // '>'
    HashCloseCurly, // '#}'

    /* Operators */
    /* Compound Assignment */
    OrAssign,           // '|='
    XorAssign,          // '^='
    AndAssign,          // '&='
    UnsignedRightShiftassign, // '>>>='
    RightShiftAssign,   // '>>='
    LeftShiftAssign,    // '<<='
    DivRemAssign,       // '%%='
    RemainderAssign,    // '%='
    DivideAssign,       // '/='
    MultiplyAssign,     // '*='
    ExponentiationAssign,// '*='
    MinusAssign,        // '-='
    PlusAssign,         // '+='
    /* Assignment */
    Assign,             // '='
    /* Ternary */
    Question,           // '?'
    /* Logical */
    OrOr,               // '||'
    AndAnd,             // '&&'
    QuestionQuestion,   // '??'
    /* Bitwise */
    Or,                 // '|'
    Xor,                // '^'
    And,                // '&'
    /* Relational */
    NotEqual,           // '!='
    Equal,              // '=='
    NotEquivalent,      // '!=='
    Equivalent,         // '==='
    GreaterOrEqual,     // '>='
    Greater,            // '>'
    LessOrEqual,        // '<='
    Less,               // '<'
    /* Shift */
    UnsignedRightShift, // '>>>'
    RightShift,         // '>>'
    LeftShift,          // '<<'
    Minus,              // '-'
    Plus,               // '+'
    DivRem,             // '%%'
    Remainder,          // '%'
    Divide,             // '/'
    Multiply,           // '*'
    Exponentiation,     // '**'

    /* Unary operators */
    /* Prefix unary operators */
    AddressOf,          // '&'
    Dereference,        // '*'
    BitwiseNot,         // '~'
    LogicalNot,         // '!'
    UnaryMinus,         // '-'
    UnaryPlus,          // '+'
    /* Prefix or suffix unary operators */
    MinusMinus,         // '--'
    PlusPlus,           // '++'
    /* Suffix unary operators */
    Pointer,            // '*'
    Reference,          // '&'

    /* Dots */
    Dot,                // '.'
    DotDot,             // '..'
    Ellipsis,           // '...'
    /* Quotes */
    SingleQuote,        // "'"
    DoubleQuote,        // '"'
    WideSingleQuote,    // "w'"
    WideDoubleQuote,    // 'w"'
    RawSingleQuote,     // "r'"
    RawDoubleQuote,     // 'r"'

    /* Comments */
    OpenSingleLineComment,  // '//'
    OpenMultiLineComment,   // '/*'
    CloseSingleLinecomment, // '*/'

    /* Number */
    DecimalNumber,
    HexadecimalNumber,
    BinaryNumber,
    OctalNumber,
    FloatNumber,

    /* Text */
    Text,
};

struct SourceChar;
struct SourceRange;
struct SourceToken;
struct SourceFile;

struct SourceChar {
    const char *pos{};
    const int   line{};
    const int   col{};

    int length() const;
    bool isValid(int length) const;
    bool isAlpha() const;
    bool isDigit() const;
    bool isAlphaNumeric() const;
};

struct SourceRange {
    SourceChar start{};
    SourceChar end{};

    int length() const;
};

struct SourceToken {
    SourceRange range{};
    TokenKind   kind{};
};

struct SourceFile {
    String            source{};
    List<SourceToken> tokens{};
};

struct SourceFolder {
    List<SourceFile>   files{};
    List<SourceFolder> folders{};
};

struct SourceTree {
    SourceFolder *root;
};

} // namespace exy

#endif // SOURCE_H_