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
    void readWhiteSpace(Stream);
    void readNumber(Stream);
    void readPrefixedHex(Stream);
    void readPrefixedBin(Stream);
    void readPrefixedOct(Stream);
    void readDecimal(Stream);
    void continueFromExponent(Stream);
    void continueFromIntSuffix(Stream, Tok);
    void continueFromFloatSuffix(Stream);
    void continueFromHexSuffix(Stream);

    void take(Stream, Tok);

    static bool isaDigit(Pos);
    static bool isaHex(Pos);
    static bool isaHexOrBlank(Pos);
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
};
} // namespace exy