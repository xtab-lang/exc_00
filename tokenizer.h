#pragma once

namespace exy {
struct SourceCharStream;
struct Tokenizer {
    SourceFile        &file;
    List<SourceToken> &tokens;

    Tokenizer(SourceFile &file);

    void run();

    static INT lengthOf(const CHAR);

private:
    using Stream = SourceCharStream&;
    using    Pos = const SourceChar*;
    Pos pos = nullptr;
    void read(Stream);
    void readText(Stream);
    void readPunctuation(Stream);
    void readWhiteSpace(Stream, Tok);
    void readNumber(Stream);
    bool tryDecOrFloat(Stream);
    bool tryHex(Stream);
    bool tryBin(Stream);
    bool tryOct(Stream);

    bool continueDecimalFromDot(Stream);

    bool tryExponent(Stream, Pos dot);
    bool tryIntSuffix(Stream, Tok);
    bool tryFloatSuffix(Stream, Tok);

    void take(Stream, Tok);

    static bool isaDigit(Pos);
    static bool isaDigitOrBlank(Pos);
    static bool isaHex(Pos);
    static bool isaHexOrBlank(Pos);
    static bool isaHexLetter(Pos);
    static bool isaHexPrefix(Pos);
    static bool isaBin(Pos);
    static bool isaBinOrBlank(Pos);
    static bool isanOct(Pos);
    static bool isanOctOrBlank(Pos);
    static bool isAlpha(Pos);
    static bool isAlphaNumeric(Pos);
    static bool isIntSuffix(Pos);
    static bool isHexFloatSuffix(Pos);
    static bool isFloatSuffix(Pos);
    static bool isExponent(Pos);
    static bool isHexSuffix(Pos);
    static bool isBinSuffix(Pos);
    static bool isOctSuffix(Pos);
    static bool isSign(Pos);
    static bool isZero(Pos);
};
} // namespace exy