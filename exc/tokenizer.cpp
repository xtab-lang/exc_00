#include "pch.h"
#include "tokenizer.h"

#include "preparser.h"
#include "source.h"
#include "compiler.h"

namespace exy {
//------------------------------------------------------------------------------------------------
#define err(startChar, endChar, msg, ...) \
    tokenizerError((file), (startChar), (endChar), __FILE__, __FUNCTION__, __LINE__, msg, __VA_ARGS__)
static void tokenizerError(const SourceFile &sourceFile,  const SourceChar *start, const SourceChar *end,
                           const char *cppFile, const char *cppFn, int cppLine, 
                           const char *msg, ...) {
    UNREFERENCED_PARAMETER(sourceFile);
    UNREFERENCED_PARAMETER(start);
    UNREFERENCED_PARAMETER(end);
    UNREFERENCED_PARAMETER(cppFile);
    UNREFERENCED_PARAMETER(cppFn);
    UNREFERENCED_PARAMETER(cppLine);
    UNREFERENCED_PARAMETER(msg);
    Assert(0);
}
//------------------------------------------------------------------------------------------------
struct CharStream {
    const char *pos;
    const char *end;
    int         line = 1;
    int         col = 1;

    CharStream(SourceFile &file) : pos(file.source.text), end(file.source.end()) {}

    auto next() {
        Assert(pos <= end);
        SourceChar ch{ pos, line, col };
        if (pos < end) {
            if (*pos == '\n') {
                ++pos;
                ++line;
                col = 1;
            } else {
                ++pos;
                ++col;
            }
        }
        return ch;
    }
    auto isEOF(const SourceChar &ch) {
        return ch.pos == end;
    }
};
//------------------------------------------------------------------------------------------------
constexpr auto lengthOf(const SourceChar *ch) {
    auto n = (int)*ch->pos;
    if ((n & 0x80) == 0) {
        return 1;
    } if ((n & 0xE0) == 0xC0) {
        return 2;
    } if ((n & 0xF0) == 0xE0) {
        return 3;
    } if ((n & 0xF8) == 0xF0) {
        return 4;
    }
    return 0;
}
constexpr auto isAlpha(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || ch == '$';
}
constexpr auto isAlpha(const SourceChar *ch) {
    auto c = *ch->pos;
    return isAlpha(c) || lengthOf(ch) > 1;
}
constexpr auto isAlphaNumeric(const SourceChar *ch) {
    auto c = *ch->pos;
    return isAlpha(c) || (c >= '0' && c <= '9') || lengthOf(ch) > 1;
}
constexpr auto isNotAlpha(char ch) {
    return !isAlpha(ch);
}
constexpr auto isNotAlpha(const SourceChar *ch) {
    return !isAlpha(ch);
}
constexpr auto isNotAlphaNumeric(const SourceChar *ch) {
    return !isAlphaNumeric(ch);
}
constexpr auto isDecimal(char ch) {
    return ch >= '0' && ch <= '9';
}
constexpr auto isNotDecimal(char ch) {
    return ch < '0' || ch > '9';
}
constexpr auto isDecimalOrUnderscore(char ch) {
    return (ch >= '0' && ch <= '9') || ch == '_';
}
constexpr auto isHexadecimal(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'Z');
}
constexpr auto isNotHexadecimal(char ch) {
    return !isHexadecimal(ch);
}
constexpr auto isHexadecimalOrUnderscore(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'Z') ||
        ch == '_';
}
constexpr auto isBinary(char ch) {
    return ch == '0' || ch == '1';
}
constexpr auto isNotBinary(char ch) {
    return ch != '0' && ch != '1';
}
constexpr auto isBinaryOrUnderscore(char ch) {
    return ch == '0' || ch == '1' || ch == '_';
}
constexpr auto isOctal(char ch) {
    return ch >= '0' && ch <= '7';
}
constexpr auto isNotOctal(char ch) {
    return ch < '0' || ch > '7';
}
constexpr auto isOctalOrUnderscore(char ch) {
    return (ch >= '0' && ch <= '7') || ch == '_';
}
//------------------------------------------------------------------------------------------------
struct TokenStream {
    const SourceFile &file;
    const SourceChar *mark;
    const SourceChar *cur;
    const SourceChar *end;

    TokenStream(const SourceFile &file, const List<SourceChar> &list) : file(file), 
        mark(&list.first()), cur(&list.first()), end(&list.last()) {}

    SourceToken next() {
        Assert(mark <= cur);
        Assert(cur <= end);
        mark = cur;
        if (cur == end) {
            return make(Tok::EndOfFile);
        }
        auto length = lengthOf(cur);
        if (length > 0) {
            return make(read());
        }
        return skipZeroLength();
    }

    SourceToken skipZeroLength() {
        while (++cur < end && lengthOf(cur) == 0);
        return make(Tok::Unknown);
    }

    Tok read() {
        auto kind = skipWhiteSpace();
        if (kind != Tok::Unknown) {
            return kind;
        }
        Assert(mark == cur);
        ++cur; // Past current {mark}
        switch (auto ch = *mark->pos) {
            case '\\': return Tok::BackSlash;
            case ',': return Tok::Comma;
            case '(': return Tok::OpenParen;
            case '[': return Tok::OpenBracket;
            case '{': return Tok::OpenCurly;
            case ')': return Tok::CloseParen;
            case ']': return Tok::CloseBracket;
            case '\'': return Tok::SingleQuote;
            case '"': return Tok::DoubleQuote;
            case ';': return Tok::SemiColon;
            case '@': if (cur < end) {
                if (*cur->pos == '@') return move(Tok::AtAt);
            } return Tok::At;
            case ':': if (cur < end) {
                if (*cur->pos == ':') return move(Tok::ColonColon);
            } return Tok::Colon;
            case '}': if (cur < end) {
                if (*cur->pos == '#') return move(Tok::CloseCurlyHash);
            } return Tok::CloseCurly;
            case '<': if (cur < end) {
                if (*cur->pos == '<') {
                    ++cur; // Past second '<'
                    if (cur < end && *cur->pos == '=') return move(Tok::LeftShiftAssign);
                    return Tok::LeftShift;
                }
                if (*cur->pos == '=') return move(Tok::LessOrEqual);
            }  return Tok::Less;
            case '>': if (cur < end) {
                if (*cur->pos == '>') {
                    ++cur; // Past second '>'
                    if (cur < end && *cur->pos == '>') {
                        ++cur; // Past third '>'
                        if (cur < end && *cur->pos == '=') return move(Tok::UnsignedRightShiftAssign);
                        return Tok::UnsignedRightShift;
                    }
                    return Tok::RightShift;
                }
                if (*cur->pos == '=') return move(Tok::GreaterOrEqual);
            } return Tok::Greater;
            case '#': if (cur < end) {
                if (*cur->pos == '#') return move(Tok::HashHash);
                if (*cur->pos == '(') return move(Tok::HashOpenParen);
                if (*cur->pos == '[') return move(Tok::HashOpenBracket);
                if (*cur->pos == '{') return move(Tok::HashOpenCurly);
            } return Tok::Hash;
            case '|': if (cur < end) {
                if (*cur->pos == '=') return move(Tok::OrAssign);
                if (*cur->pos == '|') return move(Tok::OrOr);
            } return Tok::Or;
            case '^': if (cur < end) {
                if (*cur->pos == '=') return move(Tok::XOrAssign);
            } return Tok::XOr;
            case '&': if (cur < end) {
                if (*cur->pos == '=') return move(Tok::AndAssign);
                if (*cur->pos == '&') return move(Tok::AndAnd);
            } return Tok::And;
            case '%': if (cur < end) {
                if (*cur->pos == '=') return move(Tok::RemainderAssign);
                if (*cur->pos == '%') {
                    ++cur;
                    if (cur < end && *cur->pos == '=') return move(Tok::DivRemAssign);
                    return Tok::DivRem;
                }
            } return Tok::Remainder;
            case '/': if (cur < end) {
                if (*cur->pos == '=') return move(Tok::DivideAssign);
                if (*cur->pos == '/') return move(Tok::OpenSingleLineComment);
                if (*cur->pos == '*') return move(Tok::OpenMultiLineComment);
            } return Tok::Divide;
            case '*': if (cur < end) {
                if (*cur->pos == '=') return move(Tok::MultiplyAssign);
                if (*cur->pos == '*') {
                    ++cur;
                    if (cur < end && *cur->pos == '=') return move(Tok::ExponentiationAssign);
                    return Tok::Exponentiation;
                } 
                if (*cur->pos == '/') return move(Tok::CloseMultiLineComment);
            } return Tok::Multiply;
            case '-': if (cur < end) {
                if (*cur->pos == '=') return move(Tok::MinusAssign);
                if (*cur->pos == '-') return move(Tok::MinusMinus);
            } return Tok::Minus;
            case '+': if (cur < end) {
                if (*cur->pos == '=') return move(Tok::PlusAssign);
                if (*cur->pos == '+') return move(Tok::PlusPlus);
            } return Tok::Plus;
            case '?': if (cur < end) {
                if (*cur->pos == '?') return move(Tok::QuestionQuestion);
            } return Tok::Question;
            case '!': if (cur < end) {
                if (*cur->pos == '=') {
                    ++cur;
                    if (cur < end && *cur->pos == '=') return move(Tok::NotEquivalent);
                    return Tok::NotEqual;
                }
                if (*cur->pos == '<') return move(Tok::GreaterOrEqual);
                if (*cur->pos == '>') return move(Tok::LessOrEqual);
            } return Tok::LogicalNot;
            case '=': if (cur < end) {
                if (*cur->pos == '=') {
                    ++cur;
                    if (cur < end && *cur->pos == '=') return move(Tok::Equivalent);
                    return Tok::Equal;
                }
            } return Tok::Assign;
            case '.': if (cur < end) {
                if (*cur->pos == '.') {
                    ++cur;
                    if (cur < end && *cur->pos == '.') return move(Tok::Ellipsis);
                    return Tok::DotDot;
                }
            } return Tok::Dot;
            case 'w': if (cur < end) {
                if (*cur->pos == '\'') return move(Tok::WideSingleQuote);
                if (*cur->pos == '"')  return move(Tok::WideDoubleQuote);
            } break;
            case 'r': if (cur < end) {
                if (*cur->pos == '\'') return move(Tok::RawSingleQuote);
                if (*cur->pos == '"')  return move(Tok::RawDoubleQuote);
            } break;
            default: if (isDecimal(ch)) {
                return readNumber();
            } else if (isAlpha(mark)) {
                return readText();
            } break;
        }
        return Tok::Unknown;
    }

    Tok skipWhiteSpace() {
        auto sps = 0, crs = 0, lfs = 0;
        while (cur < end) {
            auto moved = 0;
            while (cur < end && (*cur->pos == ' ' || *cur->pos == '\t')) { ++cur; ++sps; ++moved; }
            while (cur < end && *cur->pos == '\r') { ++cur; ++crs; ++moved; }
            while (cur < end && *cur->pos == '\n') { ++cur; ++lfs; ++moved; }
            if (!moved) break;
        } if (lfs) {
            return Tok::NewLine;
        } if (sps) {
            return Tok::Space;
        }
        return Tok::Unknown;
    }

    Tok readNumber() {
        auto firstDigit = *mark->pos;
        if (cur == end) return Tok::Decimal;
        if (firstDigit == '0') {
            auto ch = *cur->pos;
            if (ch == 'x' || ch == 'X') {
                return readNumberWithHexadecimalPrefix();
            } if (ch == 'b' || ch == 'B') {
                return readNumberWithBinaryPrefix();
            } if (ch == 'o' || ch == 'O') {
                return readNumberWithOctalPrefix();
            }
        }
        for (; cur < end && isDecimalOrUnderscore(*cur->pos); ++cur);
        if (cur == end) return Tok::Decimal;
        if (*cur->pos == '.') return readFloatFromDot();
        if (*cur->pos == 'e' || *cur->pos == 'E') readFloatFromExponent(nullptr);
        return readDecimalSuffix();
    }

    Tok readNumberWithHexadecimalPrefix() {
        if (++cur == end) return Tok::Text;
        if (isNotHexadecimal(*cur->pos)) return readText();
        for (++cur; cur < end && isHexadecimalOrUnderscore(*cur->pos); ++cur);
        if (cur == end) return Tok::Text;
        auto ch = *cur->pos;
        if (ch == 'u' || ch == 'U' || ch == 'i' || ch == 'I') {
            return readIntegerSuffix(Tok::Hexadecimal);
        } if (isAlphaNumeric(cur)) {
            return readText();
        }
        return Tok::Hexadecimal;
    }

    Tok readNumberWithBinaryPrefix() {
        if (++cur == end) return Tok::Text;
        if (isNotBinary(*cur->pos)) return readText();
        for (++cur; cur < end && isBinaryOrUnderscore(*cur->pos); ++cur);
        if (cur == end) return Tok::Text;
        auto ch = *cur->pos;
        if (ch == 'u' || ch == 'U' || ch == 'i' || ch == 'I') {
            return readIntegerSuffix(Tok::Binary);
        } if (isAlphaNumeric(cur)) {
            return readText();
        }
        return Tok::Binary;
    }

    Tok readNumberWithOctalPrefix() {
        if (++cur == end) return Tok::Text;
        if (isNotOctal(*cur->pos)) return readText();
        for (++cur; cur < end && isOctalOrUnderscore(*cur->pos); ++cur);
        if (cur == end) return Tok::Text;
        auto ch = *cur->pos;
        if (ch == 'u' || ch == 'U' || ch == 'i' || ch == 'I') {
            return readIntegerSuffix(Tok::Octal);
        } if (isAlphaNumeric(cur)) {
            return readText();
        }
        return Tok::Octal;
    }

    Tok readFloatFromDot() {
        auto dot = cur++; // One past '.'
        if (cur == end || isNotDecimal(*cur->pos)) {
            cur = dot; // Restore to '.'
            return Tok::Decimal;
        }
        for (++cur; cur < end && isDecimalOrUnderscore(*cur->pos); ++cur);
        if (cur == end) return Tok::Float;
        auto ch = *cur->pos;
        if (ch == 'e' || ch == 'E') return readFloatFromExponent(dot);
        if (ch == 'f' || ch == 'F') return readFloatSuffix(dot);
        if (isAlphaNumeric(cur)) {
            cur = dot;
            return Tok::Text;
        }
        return Tok::Float;
    }

    Tok readFloatFromExponent(const SourceChar *dot) {
        auto sign = cur++;
        if (cur == end) return Tok::Float;
        auto ch = *cur->pos;
        if (ch == '-' || ch == '+') {
            if (cur == end) {
                if (dot) {
                    cur = dot;
                    return Tok::Decimal;
                }
                cur = sign;
                return Tok::Text;
            }
        } if (isNotDecimal(*cur->pos)) {
            if (dot) {
                cur = dot;
                return Tok::Decimal;
            }
            cur = sign;
            return readText();
        }
        for (++cur; cur < end && isDecimalOrUnderscore(*cur->pos); ++cur);
        if (cur == end) return Tok::Float;
        ch = *cur->pos;
        if (ch == 'f' || ch == 'F') return readFloatSuffix(dot);
        if (isAlpha(cur)) {
            if (dot) {
                cur = dot;
                return Tok::Decimal;
            }
            cur = sign;
            return readText();
        }
        return Tok::Float;
    }

    Tok readDecimalSuffix() {
        auto ch = *cur->pos;
        if (ch == 'u' || ch == 'U' || ch == 'i' || ch == 'I') {
            return readIntegerSuffix(Tok::Decimal);
        } if (ch == 'f' || ch == 'F') {
            return readFloatSuffix(nullptr);
        } if (ch == 'h' || ch == 'H') {
            return readHexadecimalSuffix();
        } if (ch == 'b' || ch == 'B') {
            return readBinarySuffix();
        } if (ch == 'o' || ch == 'O') {
            return readOctalSuffix();
        } if (isAlpha(cur)) {
            return readText();
        }
        return Tok::Decimal;
    }

    Tok readHexadecimalSuffix() {
        if (++cur == end) return Tok::Hexadecimal;
        auto ch = *cur->pos;
        if (ch == 'u' || ch == 'U' || ch == 'i' || ch == 'I') {
            return readIntegerSuffix(Tok::Hexadecimal);
        } if (isAlphaNumeric(cur)) {
            return readText();
        }
        return Tok::Hexadecimal;
    }

    Tok readBinarySuffix() {
        if (++cur == end) return Tok::Binary;
        auto ch = *cur->pos;
        if (ch == 'u' || ch == 'U' || ch == 'i' || ch == 'I') {
            return readIntegerSuffix(Tok::Binary);
        } if (isAlphaNumeric(cur)) {
            return readText();
        }
        return Tok::Binary;
    }

    Tok readOctalSuffix() {
        if (++cur == end) return Tok::Octal;
        auto ch = *cur->pos;
        if (ch == 'u' || ch == 'U' || ch == 'i' || ch == 'I') {
            return readIntegerSuffix(Tok::Octal);
        } if (isAlphaNumeric(cur)) {
            return readText();
        }
        return Tok::Octal;
    }

    Tok readIntegerSuffix(Tok token) {
        if (++cur == end) return token;
        switch (*cur->pos) {
            case '8': {
                ++cur;
                if (cur == end || isNotAlphaNumeric(cur)) return token;
            } break;
            case '1': {
                ++cur;
                if (cur == end) break;
                if (*cur->pos == '6') {
                    ++cur;
                    if (cur == end || isNotAlphaNumeric(cur)) return token;
                }
            } break;
            case '3': {
                ++cur;
                if (cur == end) break;
                if (*cur->pos == '2') {
                    ++cur;
                    if (cur == end || isNotAlphaNumeric(cur)) return token;
                }
            } break;
            case '6': {
                ++cur;
                if (cur == end) break;
                if (*cur->pos == '4') {
                    ++cur;
                    if (cur == end || isNotAlphaNumeric(cur)) return token;
                }
            } break;
            default: if (isNotAlphaNumeric(cur)) {
                return token;
            } break;
        }
        return readText();
    }

    Tok readFloatSuffix(const SourceChar *dot) {
        if (++cur == end) return Tok::Float;
        switch (*cur->pos) {
            case '3': {
                ++cur;
                if (cur == end) break;
                if (*cur->pos == '2') {
                    ++cur;
                    if (cur == end || isNotAlphaNumeric(cur)) return Tok::Float;
                }
            } break;
            case '6': {
                ++cur;
                if (cur == end) break;
                if (*cur->pos == '4') {
                    ++cur;
                    if (cur == end || isNotAlphaNumeric(cur)) return Tok::Float;
                }
            } break;
            default: if (isNotAlphaNumeric(cur)) {
                return Tok::Float;
            } break;
        } if (dot) {
            cur = dot;
        }
        return readText();
    }

    Tok readText() {
        Assert(mark < cur);
        for (; cur < end && isAlphaNumeric(cur); ++cur);
        if (cur > mark) return Tok::Text;
        return move(Tok::Unknown);
    }

    Tok move(Tok kind) {
        ++cur;
        return kind;
    }

    SourceToken make(Tok kind) {
        return { file, *mark, *cur, kind };
    }
    static auto isEOF(const SourceToken &token) {
        return token.kind == Tok::EndOfFile;
    }
};
//------------------------------------------------------------------------------------------------
struct Tokenizer {
    auto next(SourceFile &file) {
        CharStream       stream{ file };
        List<SourceChar> list{};
        while (true) {
            auto ch = stream.next();
            list.append(ch);
            if (stream.isEOF(ch)) {
                break;
            }
        }
        traceln("%s#<underline yellow> { size: %i#<bold> B, characters: %i#<bold>, thread: %i#<green> }", 
                file.path, file.source.length, list.length, GetCurrentThreadId());
        tokenize(file, list);
        list.dispose();
    }

    void tokenize(SourceFile &file, const List<SourceChar> &chars) {
        TokenStream stream{ file, chars };
        auto &tokens = file.tokens;
        while (true) {
            auto token = stream.next();
            tokens.append(token);
            if (token.kind == Tok::EndOfFile) {
                break;
            }
        }
    }
};
//------------------------------------------------------------------------------------------------
namespace tok_pass {
static void updateTokenCount(SourceFolder *folder) {
    for (auto i = 0; i < folder->folders.length; ++i) {
        updateTokenCount(folder->folders.items[i]);
    } for (auto i = 0; i < folder->files.length; ++i) {
        auto file = folder->files.items[i];
        comp.source->tokens += file->tokens.length;
    }
}

bool run() {
    traceln("\r\n%cl#<cyan|blue> { filesPerThread: %i#<magenta>, threads: %i#<magenta> }",
            S("tokenizer"), comp.options.defaultFilesPerThread, aio::ioThreads());

    SourceFileProvider provider{};
    Tokenizer          tokenizer{};
    aio::run(tokenizer, provider);
    provider.dispose();

    if (comp.errors == 0) {
        preparse();
        updateTokenCount(comp.source->root);
    }

    traceln("%cl#<cyan|blue> { totalTokens: %i#<magenta> }", S("lexer"), comp.source->tokens);

    return comp.errors == 0;
}
} // namespace tok_pass
} // namespace exy