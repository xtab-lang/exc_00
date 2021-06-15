#pragma once

//  https://blog.dyvil.org/syntax/2016/04/19/angle-bracket-generics.html

namespace exy {
struct TokenProcessor {
    List<SourceToken> &tokens;

    TokenProcessor(SourceFile &file);
    void dispose();
    void run();
private:
    List<INT> opens{};
    List<INT> openAngles{};

    enum State {
        InFile,         // code in a file but not in any enclosure
        InParens,       // code enclosed in '(' or '#('
        InBrackets,     // code enclosed in '[' or '#['
        InCurlies,      // code enclosed in '{'

        InHashCurlies,  // text enclosed in '#{'
        InSingleQuoted, // text enclosed in "'" or "w'" or "r'"
        InDoubleQuoted, // text enclosed in '"' or 'w"' or 'r"'
    };
    State getState(Tok);

    INT skipSingleLineComment(INT i);
    INT skipMultiLineComment(INT i);

    bool isaCloseAngle(INT i);
    bool isaPointerOrReference(INT i);
};
} // namespace exy