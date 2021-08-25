#include "pch.h"

namespace exy {
SourcePos& SourcePos::operator=(const SourcePos &other) {
    struct Dummy { SourceFile *file; SourceRange range; };
    auto a = (Dummy*)this;
    auto b = (Dummy*)&other;
    a->file = b->file;
    a->range = b->range;
    return *this;
}
//----------------------------------------------------------
String SourceToken::name() const {
    switch (kind) {
    #define ZM(zName, zText) case Tok::zName: return { S(#zName) };
        DeclarePunctuationTokens(ZM)
        DeclareGroupingTokens(ZM)
        DeclareOperatorTokens(ZM)
        DeclareTextTokens(ZM)
    #undef ZM
        default:
            Assert(0);
            break;
    }
    return {};
}

String SourceToken::value() const {
    if (kind >= Tok::Text) {
        return pos.range.value();
    } switch (kind) {
    #define ZM(zName, zText) case Tok::zName: return { S(zText) };
        DeclarePunctuationTokens(ZM)
        DeclareGroupingTokens(ZM)
        DeclareOperatorTokens(ZM)
        DeclareTextTokens(ZM)
    #undef ZM
        default:
            Assert(0);
            break;
    }
    return {};
}

String SourceToken::name(Tok t) {
    switch (t) {
    #define ZM(zName, zText) case Tok::zName: return { S(#zName) };
    DeclarePunctuationTokens(ZM)
        DeclareGroupingTokens(ZM)
        DeclareOperatorTokens(ZM)
        DeclareTextTokens(ZM)
    #undef ZM
        default:
            Assert(0);
            break;
    }
    return {};
}

String SourceToken::name(Keyword k) {
    switch (k) {
    #define ZM(zName, zText) case Keyword::zName: return { S(#zName) };
        DeclareKeywords(ZM)
        DeclareModifiers(ZM)
        DeclareUserDefinedTypeKeywords(ZM)
        DeclareBuiltinTypeKeywords(ZM)
        DeclareCompileTimeKeywords(ZM)
    #undef ZM
        default:
            Assert(0);
            break;
    }
    return {};
}

String SourceToken::value(Tok t) {
    if (t >= Tok::Text) {
        return { S("TEXT"), 0u };
    }
    switch (t) {
    #define ZM(zName, zText) case Tok::zName: return { S(zText) };
        DeclarePunctuationTokens(ZM)
        DeclareGroupingTokens(ZM)
        DeclareOperatorTokens(ZM)
        DeclareTextTokens(ZM)
    #undef ZM
        default:
            Assert(0);
            break;
    }
    return {};
}

String SourceToken::value(Keyword k) {
    switch (k) {
    #define ZM(zName, zText) case Keyword::zName: return { S(zText), 0u };
        DeclareKeywords(ZM)
        DeclareModifiers(ZM)
        DeclareUserDefinedTypeKeywords(ZM)
        DeclareCompileTimeKeywords(ZM)
    #undef ZM
    #define ZM(zName, zSize) case Keyword::zName: return { S(#zName), 0u };
        DeclareBuiltinTypeKeywords(ZM)
    #undef ZM
        default:
            Assert(0);
            break;
    }
    return {};
}
} // namespace exy