#pragma once

namespace exy {

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
    ZM(Union,        "union")       \
    ZM(Enum,         "enum")        \
    ZM(Lambda,       "lambda")      \
    /* Function Types */            \
    ZM(Fn,           "fn")          \
    ZM(Extern,       "extern")      \
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
    ZM(Define,      "define")          \
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
    ZM(Await,       "await")        \
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
    ZM(New,         "new")          \
    ZM(Delete,      "delete")       \
    ZM(AlignOf,     "alignof")      \
    ZM(SizeOf,      "sizeof")       \
    ZM(TypeOf,      "typeof")       \
    ZM(NameOf,      "nameof")       \
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
} // namespace exy
