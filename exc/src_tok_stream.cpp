//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-09
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "src_tok_stream.h"

#include "source.h"

namespace exy {
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
//---
constexpr auto isZero(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == '0';
}
constexpr auto isHexPrefix(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'x' || c == 'X';
}
constexpr auto isBinPrefix(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'b' || c == 'B';
}
constexpr auto isBinSuffix(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'b' || c == 'B';
}
constexpr auto isOctPrefix(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'o' || c == 'O';
}
constexpr auto isOctSuffix(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'o' || c == 'O';
}
constexpr auto isSign(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == '-' || c == '+';
}
//---
constexpr auto isIntSuffix(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'i' || c == 'I' || c == 'u' || c == 'U';
}
constexpr auto isFloatSuffix(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'f' || c == 'F';
}
constexpr auto isHexFloatSuffix(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'p' || c == 'P';
}
//---
constexpr auto isHexSuffix(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'h' || c == 'H';
}
//---
constexpr auto isDecimal(const SourceChar *ch) {
    auto c = *ch->pos;
    return (c >= '0' && c <= '9');
}
constexpr auto isNotDecimal(const SourceChar *ch) {
    return !isDecimal(ch);
}
constexpr auto isDecimalOrUnderscore(const SourceChar *ch) {
    if (isDecimal(ch)) return true;
    return *ch->pos == '_';
}
constexpr auto isNotDecimalOrUnderscore(const SourceChar *ch) {
    return !isDecimalOrUnderscore(ch);
}
//---
constexpr auto isHexadecimal(const SourceChar *ch) {
    auto c = *ch->pos;
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
constexpr auto isNotHexadecimal(const SourceChar *ch) {
    return !isHexadecimal(ch);
}
constexpr auto isHexadecimalOrUnderscore(const SourceChar *ch) {
    if (isHexadecimal(ch)) return true;
    return *ch->pos == '_';
}
constexpr auto isNotHexadecimalOrUnderscore(const SourceChar *ch) {
    return !isHexadecimalOrUnderscore(ch);
}
//---
constexpr auto isBinary(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == '0' || c == '1';
}
constexpr auto isNotBinary(const SourceChar *ch) {
    return !isBinary(ch);
}
constexpr auto isBinaryOrUnderscore(const SourceChar *ch) {
    if (isBinary(ch)) return true;
    return *ch->pos == '_';
}
constexpr auto isNotBinaryOrUnderscore(const SourceChar *ch) {
    return !isBinaryOrUnderscore(ch);
}
//---
constexpr auto isOctal(const SourceChar *ch) {
    auto c = *ch->pos;
    return c >= '0' && c <= '7';
}
constexpr auto isNotOctal(const SourceChar *ch) {
    return !isOctal(ch);
}
constexpr auto isOctalOrUnderscore(const SourceChar *ch) {
    if (isOctal(ch)) return true;
    return *ch->pos == '_';
}
constexpr auto isNotOctalOrUnderscore(const SourceChar *ch) {
    return !isOctalOrUnderscore(ch);
}
//---
constexpr auto isAlpha(const SourceChar *ch) {
    auto length = lengthOf(ch);
    if (length == 1) {
        auto c = *ch->pos;
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
    }
    return length > 0;
}
constexpr auto isAlphaNumeric(const SourceChar *ch) {
    return isAlpha(ch) || isDecimal(ch);
}
//---
constexpr auto isNotAlpha(const SourceChar *ch) {
    return !isAlpha(ch);
}
constexpr auto isNotAlphaNumeric(const SourceChar *ch) {
    return !isAlphaNumeric(ch);
}
//---
constexpr auto isExponent(const SourceChar *ch) {
    auto c = *ch->pos;
    return c == 'e' || c == 'E';
}
//------------------------------------------------------------------------------------------------
TokenStream::TokenStream(const SourceFile &file, const List<SourceChar> &list) 
    : file(file), mark(&list.first()), cur(&list.first()), end(&list.last()) {}

SourceToken TokenStream::next() {
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

SourceToken TokenStream::skipZeroLength() {
    while (++cur < end && lengthOf(cur) == 0);
    return make(Tok::Unknown);
}

Tok TokenStream::read() {
    auto kind = skipWhiteSpace();
    if (kind != Tok::Unknown) {
        return kind;
    }
    Assert(mark == cur);
    ++cur; // Past current {mark}
    switch (*mark->pos) {
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
        default: if (isDecimal(mark)) {
            return readNumber();
        } else if (isAlpha(mark)) {
            return readText();
        } break;
    }
    return Tok::Unknown;
}

Tok TokenStream::skipWhiteSpace() {
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

Tok TokenStream::readNumber() {
    while (true) {
        auto found = tryDecimal();
        if (found != Tok::Unknown) return found;
        found = tryHexadecimal();
        if (found != Tok::Unknown) return found;
        found = tryBinary();
        if (found != Tok::Unknown) return found;
        found = tryOctal();
        if (found != Tok::Unknown) return found;
        break;
    }
    cur = mark + 1;
    return readText();
}

Tok TokenStream::tryDecimal() {
    cur = mark + 1;
    // decnum := dec [dec|_]+
    for (; cur < end && isDecimalOrUnderscore(cur); ++cur);
    if (cur == end) return Tok::Decimal; // decimal := decnum
    if (isExponent(cur)) {        
        return tryExponent(nullptr); // float := decnum floatexp ...
    } if (isIntSuffix(cur)) {        
        return readSuffix(Tok::Decimal); // decimal := decnum intsfx
    } if (isFloatSuffix(cur)) {        
        return readSuffix(Tok::Float); // float := decnum floatsfx
    } if (*cur->pos == '.') {
        return tryFloat(); // float := decnum '.' ...
    } if (isNotAlpha(cur)) {
        return Tok::Decimal; // decimal := decnum
    }
    return Tok::Unknown;
}

Tok TokenStream::tryFloat() {
    // floatnum := decnum '.' decnum
    auto dot = cur++;
    if (cur == end || isNotDecimal(cur)) {
        cur = dot;
        return Tok::Decimal; // decimal := decnum
    }
    for (++cur; cur < end && isDecimalOrUnderscore(cur); ++cur);
    if (cur == end) return Tok::Float; // float := floatnum
    if (isExponent(cur)) {
        // float := decnum '.' decnum floatexp
        return tryExponent(dot);
    } if (isFloatSuffix(cur)) {
        // float := floatnum 'f' | 'F' [32|64]
        return readSuffix(Tok::Float);
    } if (isNotAlpha(cur)) {
        // float := floatnum
        return Tok::Float;
    }
    return Tok::Unknown;
}

Tok TokenStream::tryExponent(const SourceChar *dot) {
    // floatexp := 'e' | 'E' ('-'|'+')? decnum
    if (++cur == end) {
        if (dot) {
            cur = dot;
            return Tok::Decimal;
        }
        return Tok::Text;
    } if (isSign(cur)) {
        auto sign = cur++;
        if (cur == end) {
            if (dot) {
                cur = dot;
                return Tok::Decimal;
            }
            cur = sign;
            return Tok::Text;
        } if (isNotDecimal(cur)) {
            if (dot) {
                cur = dot;
                return Tok::Decimal;
            }
            cur = sign;
            return Tok::Text;
        }
    } else if (isNotDecimal(cur)) {
        if (dot) {
            cur = dot;
            return Tok::Decimal;
        }
        return Tok::Unknown;
    }
    for (++cur; cur < end && isDecimalOrUnderscore(cur); ++cur);
    if (cur == end) return Tok::Float; // float := floatnum floatexp
    if (isFloatSuffix(cur)) {        
        return readSuffix(Tok::Float); // float := floatnum floatexp floatsfx
    } if (isNotAlpha(cur)) {        
        return Tok::Float; // float := floatnum floatexp
    }
    return Tok::Unknown;
}

Tok TokenStream::tryHexadecimal() {
    // hexpref := '0x' | '0X'
    // hexnum  := hex [hex|_]+
    cur = mark;
    if (isZero(cur++)) {
        Assert(cur < end);
        if (isHexPrefix(cur++)) {
            if (cur == end) return Tok::Text;
            if (isNotHexadecimal(cur)) return Tok::Unknown;
            for (++cur; cur < end && isHexadecimalOrUnderscore(cur); ++cur);
            if (isIntSuffix(cur)) {
                return readSuffix(Tok::Hexadecimal); // hex := hexpref hexnum intsfx
            } if (isHexFloatSuffix(cur)) {
                return readSuffix(Tok::Hexadecimal); // float := hexpref hexnum hexfloatsfx
            } if (isNotAlpha(cur)) {
                return Tok::Hexadecimal; // hex := hexpref hexnum
            }
            return Tok::Unknown;
        }
    }
    return trySuffixedHexadecimal();
}

Tok TokenStream::trySuffixedHexadecimal() {
    // hexsfx  := 'h' | 'H'
    cur = mark + 1;
    for (; cur < end && isHexadecimalOrUnderscore(cur); ++cur);
    if (isHexSuffix(cur)) {
        if (++cur == end) return Tok::Hexadecimal;
        if (isIntSuffix(cur)) {
            return readSuffix(Tok::Hexadecimal); // hex := hexnum hexsfx intsfx
        } if (isFloatSuffix(cur) || isHexFloatSuffix(cur)) {
            return readSuffix(Tok::Hexadecimal); // float := hexnum hexsfx floatsfx
        } if (isNotAlpha(cur)) {
            return Tok::Hexadecimal; // hex := hexnum hexsfx
        }
    }
    return Tok::Unknown;
}

Tok TokenStream::tryBinary() {
    // binpref := '0x' | '0X'
    // binum   := bin [bin|_]+
    cur = mark;
    if (isZero(cur++)) {
        Assert(cur < end);
        if (isBinPrefix(cur++)) {
            if (cur == end) return Tok::Text;
            if (isNotBinary(cur)) return Tok::Unknown;
            for (++cur; cur < end && isBinaryOrUnderscore(cur); ++cur);
            if (isIntSuffix(cur)) {
                return readSuffix(Tok::Binary); // bin := binpref binum intsfx
            } if (isFloatSuffix(cur)) {
                return readSuffix(Tok::Binary); // float := binpref binum floatsfx
            } if (isNotAlpha(cur)) {
                return Tok::Binary; // bin := binpref binum
            }
            return Tok::Unknown;
        }
    }
    return trySuffixedBinary();
}

Tok TokenStream::trySuffixedBinary() {
    // binsfx  := 'b' | 'B'
    cur = mark;
    for (; cur < end && isBinaryOrUnderscore(cur); ++cur);
    if (isBinSuffix(cur)) {
        if (++cur == end) return Tok::Binary;
        if (isIntSuffix(cur)) {
            return readSuffix(Tok::Binary); // bin := binum binsfx intsfx
        } if (isFloatSuffix(cur)) {
            return readSuffix(Tok::Binary); // float := binum binsfx floatsfx
        } if (isNotAlpha(cur)) {
            return Tok::Binary; // bin := binum binsfx
        }
    }
    return Tok::Unknown;
}

Tok TokenStream::tryOctal() {
    // octpref := '0x' | '0X'
    // octnum   := bin [bin|_]+
    cur = mark;
    if (isZero(cur++)) {
        Assert(cur < end);
        if (isOctPrefix(cur++)) {
            if (cur == end) return Tok::Text;
            if (isNotOctal(cur)) return Tok::Unknown;
            for (++cur; cur < end && isOctalOrUnderscore(cur); ++cur);
            if (isIntSuffix(cur)) {
                return readSuffix(Tok::Octal); // oct := octpref octnum intsfx
            } if (isFloatSuffix(cur)) {
                return readSuffix(Tok::Octal); // float := octpref octnum floatsfx
            } if (isNotAlpha(cur)) {
                return Tok::Octal; // oct := octpref octnum
            }
            return Tok::Unknown;
        }
    }
    return trySuffixedOctal();
}

Tok TokenStream::trySuffixedOctal() {
    // octsfx  := 'o' | 'O'
    cur = mark;
    for (; cur < end && isOctalOrUnderscore(cur); ++cur);
    if (isOctSuffix(cur)) {
        if (++cur == end) return Tok::Octal;
        if (isIntSuffix(cur)) {
            return readSuffix(Tok::Octal); // oct := octnum octnsfx intsfx
        } if (isFloatSuffix(cur)) {
            return readSuffix(Tok::Octal); // float := octnum octsfx floatsfx
        } if (isNotAlpha(cur)) {
            return Tok::Octal; // oct := octnum binsfx
        }
    }
    return Tok::Unknown;
}

Tok TokenStream::readSuffix(Tok token) {
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
            } else if (*cur->pos == '2') {
                if (++cur == end) break;
                if (*cur->pos == '8') {
                    ++cur;
                    if (cur == end || isNotAlphaNumeric(cur)) return token;
                }
            }
        } break;
        case '2': {
            ++cur;
            if (cur == end) break;
            if (*cur->pos == '5') {
                if (++cur == end) break;
                if (*cur->pos == '6') {
                    ++cur;
                    if (cur == end || isNotAlphaNumeric(cur)) return token;
                }
            }
        } break;
        case '3': {
            if (++cur == end) break;
            if (*cur->pos == '2') {
                ++cur;
                if (cur == end || isNotAlphaNumeric(cur)) return token;
            }
        } break;
        case '5': {
            if (++cur == end) break;
            if (*cur->pos == '1') {
                if (++cur == end) break;
                if (*cur->pos == '2') {
                    ++cur;
                    if (cur == end || isNotAlphaNumeric(cur)) return token;
                }
            }
        } break;
        case '6': {
            if (++cur == end) break;
            if (*cur->pos == '4') {
                ++cur;
                if (cur == end || isNotAlphaNumeric(cur)) return token;
            }
        } break;
        default: if (isNotAlphaNumeric(cur)) {
            return token;
        } break;
    }
    return Tok::Unknown;
}

Tok TokenStream::readText() {
    Assert(mark < cur);
    for (; cur < end && isAlphaNumeric(cur); ++cur);
    if (cur > mark) return Tok::Text;
    return move(Tok::Unknown);
}

Tok TokenStream::move(Tok kind) {
    if (cur < end) {
        ++cur;
    }
    return kind;
}

SourceToken TokenStream::make(Tok kind) {
    return { file, *mark, *cur, kind };
}
} // namespace exy