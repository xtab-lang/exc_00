#pragma once

namespace exy {
#define DeclareBuiltinTypeKeywords(ZM)   \
/* Type declaration keywords. */         \
    /* Non-SIMD types. */                \
    ZM(Void,        0)   \
    /* Signed. */            \
        ZM(Char,        1)   \
        ZM(Int8,        1)   \
        ZM(Int16,       2)   \
        ZM(Int32,       4)   \
        ZM(Int64,       8)   \
    /* Unsigned */           \
        ZM(Bool,        1)   \
        ZM(UInt8,       1)   \
        ZM(WChar,       2)   \
        ZM(UInt16,      2)   \
        ZM(Utf8,        4)   \
        ZM(UInt32,      4)   \
        ZM(UInt64,      8)   \
    /* Floating point */     \
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

#define DeclareStructureTypeKeywords(ZM) \
    ZM(Struct,       "struct")      \
    ZM(Union,        "union")

#define DeclareFunctionTypeKeywords(ZM) \
    ZM(Lambda,       "lambda")      \
    ZM(Fn,           "fn")          \
    ZM(Extern,       "extern")      \
    ZM(UrlHandler,   "urlhandler")  \
    ZM(Html,         "html")        \
    ZM(Css,          "css")         \
    ZM(Js,           "js")          \
    ZM(Json,         "json")        \
    ZM(Sql,          "sql")         \
    ZM(Blob,         "blob")

#define DeclareUserDefinedTypeKeywords(ZM) \
    /* User-Defined Types (UDTs) */ \
    ZM(Module,       "module")      \
    /* Structure Types */           \
    DeclareStructureTypeKeywords(ZM) \
    /* Enumerated type */           \
    ZM(Enum,         "enum")        \
    /* Function Types */            \
    DeclareFunctionTypeKeywords(ZM)

#define DeclareKeywords(ZM)         \
    /* Namespace usage */           \
    ZM(Import,      "import")       \
    ZM(Export,      "export")       \
    /* Alias */                     \
    ZM(Define,      "define")       \
    /* Control */                   \
    ZM(If,          "if")           \
    ZM(Else,        "else")         \
    ZM(Switch,      "switch")       \
    ZM(Case,        "case")         \
    ZM(Default,     "default")      \
    /* Defer execution */           \
    ZM(Defer,       "defer")        \
    ZM(Using,       "using")        \
    /* Assertion */                 \
    ZM(Assert,      "assert")       \
    /* Flow break */                \
    ZM(Throw,       "throw")        \
    ZM(Return,      "return")       \
    ZM(Break,       "break")        \
    ZM(Continue,    "continue")     \
    /* Loop */                      \
    ZM(For,         "for")          \
    ZM(While,       "while")        \
    ZM(Do,          "do")           \
    /* Binary functions */          \
    ZM(As,          "as")           \
    ZM(To,          "to")           \
    ZM(Is,          "is")           \
    ZM(NotIs,       "!is")          \
    ZM(NotIn,       "!in")          \
    ZM(In,          "in")           \
    /* Unary functions */           \
    ZM(AlignOf,     "alignof")      \
    ZM(SizeOf,      "sizeof")       \
    ZM(TypeOf,      "typeof")       \
    ZM(NameOf,      "nameof")       \
    ZM(New,         "new")          \
    ZM(Delete,      "delete")       \
    ZM(Atomic,      "atomic")       \
    /* Generator */                 \
    ZM(Await,       "await")        \
    ZM(Yield,       "yield")        \
    /* Literals */                  \
    ZM(Null,        "null")         \
    ZM(void_,       "void")         \
    ZM(False,       "false")        \
    ZM(True,        "true")         \
    /* Variables */                 \
    ZM(This,        "this")         \
    ZM(Super,       "super")        \
    /* Others */                    \
    ZM(From,        "from")         \
    ZM(With,        "with")

#define DeclareModifiers(ZM)        \
    /* Visibility modifiers  */     \
    ZM(Private,     "private")      \
    ZM(Internal,    "internal")     \
    ZM(Protected,   "protected")    \
    /* Scope modifiers  */          \
    ZM(Static,      "static")       \
    /* Mutability modifiers  */     \
    ZM(Const,       "const")        \
    ZM(ReadOnly,    "readonly")     \
    /* Lifetime modifiers  */       \
    ZM(Auto,        "auto")         \
    /* Info modifiers  */           \
    ZM(Var,         "var")          \
    /* Async modifiers  */          \
    ZM(Async,       "async")        \
    /* Hierarchy modifiers  */      \
    ZM(Abstract,    "abstract")     \
    ZM(Override,    "override")     \
    /* Synchronization modifiers  */\
    ZM(Synchronized,"synchronized")

#define DeclareCompileTimeKeywords(ZM)  \
    ZM(MODULE__,     "__MODULE__")   \
    ZM(FOLDER__,     "__FOLDER__")   \
    ZM(FILE__,       "__FILE__")     \
    ZM(FUNCTION__,   "__FUNCTION__") \
    ZM(LINE__,       "__LINE__")     \
    ZM(COL__,        "__COL__")

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
    _end_udts,
    _begin_compiler_keywords,
#define ZM(zName, zText) zName,
    DeclareCompileTimeKeywords(ZM)
#undef ZM
    _end_compiler_keywords,
    _begin_builtins,
#define ZM(zName, zSize) zName,
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
    _end_builtins
};

struct Keywords {
    void initialize();
    void dispose();

    Keyword get(const String&);
private:
    struct Word {
        String  name;
        Keyword value;
    };
    List<Word> list{};
};

__declspec(selectany) Keywords kws{};
} // namespace exy
