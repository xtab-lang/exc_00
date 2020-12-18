//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-05
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "parser.h"

#include "source.h"

namespace exy {
struct Part {
    const char *text;
    int         length;
    Part() = default;
    Part(const char *text, int length) : text(text), length(length) {}
    Part(const char *start, const char *end) : text(start), length(int(end - start)) {}
};

struct Number {
    Part prefix;  // '0x' | '0X' | '0b' | '0B' | '0o' | '0O'
    Part value;   // digits
    Part spec;    // 'h' | 'H' | 'b' | 'B' | 'o' | 'O'
    Part suffix;  // 'i' | 'I' | 'u' | 'U' | 'p' | 'P' ['8' | '16' | '32' | '64' | '128' | '256' | '512']
    Number() = default;
};

static auto isIntSuffix(char ch) {
    return ch == 'i' || ch == 'I' || ch == 'u' || ch == 'U';
}

static auto isFloatSuffix(char ch) {
    return ch == 'f' || ch == 'F';
}

static auto isIntOrFloatSuffix(char ch) {
    return isIntSuffix(ch) || isFloatSuffix(ch);
}

static auto isHexFloatSuffix(char ch) {
    return ch == 'p' || ch == 'P';
}

static auto isIntOrHexFloatSuffix(char ch) {
    return isIntSuffix(ch) || isHexFloatSuffix(ch);
}

static auto readDecimal(const char *start, const char *end) {
    Number num{};
    auto pos = start;
    for (; pos < end; ++pos) {
        auto ch = *pos;
        if (isIntSuffix(ch)) {
            num.value = { start, pos };
            num.suffix = { pos, end };
            return num;
        }
        Assert((ch >= '0' && ch <= '9') || ch == '_');
    }
    num.value = { start, end };
    return num;
}

static auto readHexadecimal(const char *start, const char *end) {
    Number num{};
    auto mark = start;
    auto  pos = start;
    for (; pos < end; ++pos) {
        auto ch = *pos;
        if (isIntOrHexFloatSuffix(ch)) {
            Assert(mark < pos);
            num.value = { mark, pos };
            num.suffix = { pos, end };
            return num;
        } if (ch == 'x' || ch == 'X') {
            Assert(pos - mark == 1 && *mark == '0');
            num.prefix = { mark, pos };
            mark = pos + 1;
            continue;
        } if (ch == 'h' || ch == 'H') {
            Assert(mark == start && mark < pos);
            num.value = { mark, pos };
            num.spec = { pos, 1 };
            if (pos + 1 == end) {
                return num;
            }
            Assert(isIntOrHexFloatSuffix(pos[1]));
            num.suffix = { pos + 1, end };
            return num;
        }
        Assert((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F') || ch == '_');
    }
    Assert(mark < end);
    num.value = { mark, end };
    return num;
}

static auto readBinary(const char *start, const char *end) {
    Number num{};
    auto mark = start;
    auto  pos = start;
    for (; pos < end; ++pos) {
        auto ch = *pos;
        if (isIntOrFloatSuffix(ch)) {
            Assert(mark < pos);
            num.value = { mark, pos };
            num.suffix = { pos, end };
            return num;
        } if (ch == 'b' || ch == 'B') {
            Assert(mark < pos);
            if (pos + 1 == end) {
                num.value = { mark, pos };
                num.spec = { pos, 1 };
                return num;
            } if (isIntOrFloatSuffix(pos[1])) {
                num.value = { mark, pos };
                num.spec = { pos, 1 };
                num.suffix = { pos + 1, end };
                return num;
            } 
            Assert(pos - mark == 1 && *mark == '0');
            num.prefix = { mark, pos };
            mark = pos + 1;
            continue;
        }
        Assert(ch == '0' || ch == '1' || ch == '_');
    }
    num.value = { mark, end };
    return num;
}

static auto readOctal(const char *start, const char *end) {
    Number num{};
    auto mark = start;
    auto  pos = start;
    for (; pos < end; ++pos) {
        auto ch = *pos;
        if (isIntOrFloatSuffix(ch)) {
            Assert(mark < pos);
            num.value = { mark, pos };
            num.suffix = { pos, end };
            return num;
        } if (ch == 'o' || ch == 'O') {
            Assert(mark < pos);
            if (pos + 1 == end) {
                num.value = { mark, pos };
                num.spec = { pos, 1 };
                return num;
            } if (isIntOrFloatSuffix(pos[1])) {
                num.value = { mark, pos };
                num.spec = { pos, 1 };
                num.suffix = { pos + 1, end };
                return num;
            } 
            Assert(pos - mark == 1 && *mark == '0');
            num.prefix = { mark, pos };
            mark = pos + 1;
            continue;
        }
        Assert((ch >= '0' && ch <= '7') || ch == '_');
    }
    num.value = { mark, end };
    return num;
}

static auto readFloat(const char *start, const char *end) {
    Number num{};
    auto pos = start;
    for (; pos < end; ++pos) {
        auto ch = *pos;
        if (isFloatSuffix(ch)) {
            num.value = { start, pos };
            num.suffix = { pos, end };
            return num;
        } if (ch == '.') {
            continue;
        } if (ch == 'e' || ch == 'E') {
            continue;
        } if (ch == '-' || ch == '+') {
            continue;
        }
        Assert((ch >= '0' && ch <= '9') || ch == '_');
    }
    num.value = { start, end };
    return num;
}

static auto put(Token pos, const Part &part) {
    auto cap = numbufcap - 1;
    Assert(part.length);
    if (part.length >= cap) {
        err(pos, "number too long for numbuf: length = %i#<yellow>, numbufcap = %i#<green>", part.length, cap);
        return 0;
    }
    auto len = 0;
    for (auto i = 0; i < part.length; ++i) {
        auto ch = part.text[i];
        if (ch == '_') {
            continue;
        } if (ch == '0' && !len) {
            continue;
        }
        numbuf[len++] = ch;
    } if (!len) {
        numbuf[len++] = '0';
    }
    numbuf[len] = '\0';
    return len;
}

enum class Suffix {
    Error, None,
    i, i8, i16, i32, i64, i128, i256, i512,
    u, u8, u16, u32, u64, u128, u256, u512,
    f32, f64
};

static auto readSuffix(const Part &part) {
    if (!part.length) {
        return Suffix::None;
    }
    auto pos = part.text + 1;
    auto end = part.text + part.length;
    Assert(pos <= end);
    switch (part.text[0]) {
        case 'i': case 'I': {
            if (pos == end)  return Suffix::i32;
            if (pos + 1 == end && pos[0] == '8') return Suffix::i8;
            if (pos + 2 == end) {
                if (pos[0] == '1' && pos[1] == '6') return Suffix::i16;
                if (pos[0] == '3' && pos[1] == '2') return Suffix::i32;
                if (pos[0] == '6' && pos[1] == '4') return Suffix::i64;
            } else if (pos + 3 == end) {
                if (pos[0] == '1' && pos[1] == '2' && pos[2] == '8') return Suffix::i128;
                if (pos[0] == '2' && pos[1] == '5' && pos[2] == '6') return Suffix::i256;
                if (pos[0] == '5' && pos[1] == '1' && pos[2] == '2') return Suffix::i512;
            }
        } break;
        case 'u': case 'U': {
            if (pos == end)  return Suffix::u32;
            if (pos + 1 == end && pos[0] == '8') return Suffix::u8;
            if (pos + 2 == end) {
                if (pos[0] == '1' && pos[1] == '6') return Suffix::u16;
                if (pos[0] == '3' && pos[1] == '2') return Suffix::u32;
                if (pos[0] == '6' && pos[1] == '4') return Suffix::u64;
            } else if (pos + 3 == end) {
                if (pos[0] == '1' && pos[1] == '2' && pos[2] == '8') return Suffix::u128;
                if (pos[0] == '2' && pos[1] == '5' && pos[2] == '6') return Suffix::u256;
                if (pos[0] == '5' && pos[1] == '1' && pos[2] == '2') return Suffix::u512;
            }
        } break;
        case 'f': case 'F': case 'p': case 'P': {
            if (pos == end)  return Suffix::f32;
            if (pos + 2 == end) {
                if (pos[0] == '3' && pos[1] == '2') return Suffix::f32;
                if (pos[0] == '6' && pos[1] == '4') return Suffix::f64;
            }
        } break;
    }
    return Suffix::Error;
}

static auto isSigned(Suffix sfx) {
    return sfx >= Suffix::None && sfx <= Suffix::i512;
}

static auto isUnsigned(Suffix sfx) {
    return sfx >= Suffix::u8 && sfx <= Suffix::u512;
}

static auto decimalLengthFitsSuffix(int length, Suffix sfx) {
#define MAXINT8_DIGITS   3
#define MAXINT16_DIGITS  5
#define MAXINT32_DIGITS  10
#define MAXINT64_DIGITS  20
#define MAXINT128_DIGITS 39
#define MAXINT256_DIGITS 78
#define MAXINT512_DIGITS 155
    switch (sfx) {
        case Suffix::None:                    return length <= MAXINT64_DIGITS;
        case Suffix::i8:   case Suffix::u8:   return length <= MAXINT8_DIGITS;
        case Suffix::i16:  case Suffix::u16:  return length <= MAXINT16_DIGITS;
        case Suffix::i32:  case Suffix::u32:  return length <= MAXINT32_DIGITS;
        case Suffix::i64:  case Suffix::u64:  return length <= MAXINT64_DIGITS;
        case Suffix::i128: case Suffix::u128: return length <= MAXINT128_DIGITS;
        case Suffix::i256: case Suffix::u256: return length <= MAXINT256_DIGITS;
        case Suffix::i512: case Suffix::u512: return length <= MAXINT512_DIGITS;
    }
    Unreachable();
}

static auto hexadecimalLengthFitsSuffix(int length, Suffix sfx) {
#define MAXHEX8_DIGITS   2
#define MAXHEX16_DIGITS  4
#define MAXHEX32_DIGITS  8
#define MAXHEX64_DIGITS  16
#define MAXHEX128_DIGITS 32
#define MAXHEX256_DIGITS 64
#define MAXHEX512_DIGITS 128
    switch (sfx) {
        case Suffix::None:                    return length <= MAXHEX64_DIGITS;
        case Suffix::i8:   case Suffix::u8:   return length <= MAXHEX8_DIGITS;
        case Suffix::i16:  case Suffix::u16:  return length <= MAXHEX16_DIGITS;
        case Suffix::i32:  case Suffix::u32: case Suffix::f32: return length <= MAXHEX32_DIGITS;
        case Suffix::i64:  case Suffix::u64: case Suffix::f64: return length <= MAXHEX64_DIGITS;
        case Suffix::i128: case Suffix::u128: return length <= MAXHEX128_DIGITS;
        case Suffix::i256: case Suffix::u256: return length <= MAXHEX256_DIGITS;
        case Suffix::i512: case Suffix::u512: return length <= MAXHEX512_DIGITS;
    }
    Unreachable();
}

static auto binaryLengthFitsSuffix(int length, Suffix sfx) {
#define MAXBIN8_DIGITS   8
#define MAXBIN16_DIGITS  16
#define MAXBIN32_DIGITS  32
#define MAXBIN64_DIGITS  64
#define MAXBIN128_DIGITS 128
#define MAXBIN256_DIGITS 256
#define MAXBIN512_DIGITS 512
    switch (sfx) {
        case Suffix::None:                    return length <= MAXBIN64_DIGITS;
        case Suffix::i8:   case Suffix::u8:   return length <= MAXBIN8_DIGITS;
        case Suffix::i16:  case Suffix::u16:  return length <= MAXBIN16_DIGITS;
        case Suffix::i32:  case Suffix::u32: case Suffix::f32: return length <= MAXBIN32_DIGITS;
        case Suffix::i64:  case Suffix::u64: case Suffix::f64: return length <= MAXBIN64_DIGITS;
        case Suffix::i128: case Suffix::u128: return length <= MAXBIN128_DIGITS;
        case Suffix::i256: case Suffix::u256: return length <= MAXBIN256_DIGITS;
        case Suffix::i512: case Suffix::u512: return length <= MAXBIN512_DIGITS;
    }
    Unreachable();
}

static auto octalLengthFitsSuffix(int length, Suffix sfx) {
#define MAXOCT8_DIGITS   3
#define MAXOCT16_DIGITS  8
#define MAXOCT32_DIGITS  11
#define MAXOCT64_DIGITS  22
#define MAXOCT128_DIGITS 22 // TODO: How long is a 128 octal number?
#define MAXOCT256_DIGITS 22 // TODO: How long is a 256 octal number?
#define MAXOCT512_DIGITS 22 // TODO: How long is a 512 octal number?
    switch (sfx) {
        case Suffix::None:                    return length <= MAXOCT64_DIGITS;
        case Suffix::i8:   case Suffix::u8:   return length <= MAXOCT8_DIGITS;
        case Suffix::i16:  case Suffix::u16:  return length <= MAXOCT16_DIGITS;
        case Suffix::i32:  case Suffix::u32: case Suffix::f32: return length <= MAXOCT32_DIGITS;
        case Suffix::i64:  case Suffix::u64: case Suffix::f64: return length <= MAXOCT64_DIGITS;
        case Suffix::i128: case Suffix::u128: return length <= MAXOCT128_DIGITS;
        case Suffix::i256: case Suffix::u256: return length <= MAXOCT256_DIGITS;
        case Suffix::i512: case Suffix::u512: return length <= MAXOCT512_DIGITS;
    }
    Unreachable();
}

static auto floatLengthFitsSuffix(int length, Suffix sfx) {
    UNREFERENCED_PARAMETER(sfx); // TODO: How long is a float?
    return length < _CVTBUFSIZE;
}

static Node makeInt(Parser &p, Token pos, Node modifiers, INT64 i64, Number num, Suffix sfx) {
    switch (sfx) {
        case Suffix::None: if (i64 < INT64(MININT32) || i64 > INT64(MAXINT32)) {
            return p.mem.New<SyntaxInt64>(*pos, modifiers, i64);
        } return p.mem.New<SyntaxInt32>(*pos, modifiers, INT32(i64));
        case Suffix::i8: if (i64 < INT64(MININT8) || i64 > INT64(MAXINT8)) {
            err(pos, "%i64#<yellow> overflows the suffix %s#<yellow>", i64, &num.suffix);
            return p.empty(pos, modifiers);
        } return p.mem.New<SyntaxInt8>(*pos, modifiers, __int8(i64));
        case Suffix::i16: if (i64 < INT64(MININT16) || i64 > INT64(MAXINT16)) {
            err(pos, "%i64#<yellow> overflows the suffix %s#<yellow>", i64, &num.suffix);
            return p.empty(pos, modifiers);
        } return p.mem.New<SyntaxInt16>(*pos, modifiers, __int16(i64));
        case Suffix::i32: if (i64 < INT64(MININT32) || i64 > INT64(MAXINT32)) {
            err(pos, "%i64#<yellow> overflows the suffix %s#<yellow>", i64, &num.suffix);
            return p.empty(pos, modifiers);
        } return p.mem.New<SyntaxInt32>(*pos, modifiers, INT32(i64));
        case Suffix::i64:
            return p.mem.New<SyntaxInt64>(*pos, modifiers, i64);
        case Suffix::i128:
        case Suffix::i256:
        case Suffix::i512: {
            err(pos, "suffix not implemented: %s#<red>", &num.suffix);
            return p.empty(pos, modifiers);
        }
        default: Unreachable();
    }
}

static Node makeUInt(Parser &p, Token pos, Node modifiers, UINT64 u64, Number num, Suffix sfx) {
    switch (sfx) {
        case Suffix::None: if (u64 > UINT64(MAXUINT32)) {
            return p.mem.New<SyntaxUInt64>(*pos, modifiers, u64);
        } return p.mem.New<SyntaxUInt32>(*pos, modifiers, UINT32(u64));
        case Suffix::u8: if (u64 > UINT64(MAXUINT8)) {
            err(pos, "%ui64#<yellow> overflows the suffix %s#<yellow>", u64, &num.suffix);
            return p.empty(pos, modifiers);
        } return p.mem.New<SyntaxUInt8>(*pos, modifiers, UINT8(u64));
        case Suffix::u16: if (u64 > INT64(MAXUINT16)) {
            err(pos, "%ui64#<yellow> overflows the suffix %s#<yellow>", u64, &num.suffix);
            return p.empty(pos, modifiers);
        } return p.mem.New<SyntaxUInt16>(*pos, modifiers, UINT16(u64));
        case Suffix::u32: if (u64 > INT64(MAXUINT32)) {
            err(pos, "%ui64#<yellow> overflows the suffix %s#<yellow>", u64, &num.suffix);
            return p.empty(pos, modifiers);
        } return p.mem.New<SyntaxUInt32>(*pos, modifiers, UINT32(u64));
        case Suffix::u64:
            return p.mem.New<SyntaxUInt64>(*pos, modifiers, u64);
        case Suffix::u128:
        case Suffix::u256:
        case Suffix::u512: {
            err(pos, "suffix not implemented: %s#<red>", &num.suffix);
            return p.empty(pos, modifiers);
        }
        default: Unreachable();
    }
}

static Node reinterpretFloat(Parser &p, Token pos, Node modifiers, UINT64 u64, Number num, Suffix sfx) {
    switch (sfx) {
        case Suffix::f32: {
            if (u64 > UINT64(MAXUINT32)) {
                err(pos, "%ui64#<yellow> overflows the suffix %s#<yellow>", u64, &num.suffix);
                return p.empty(pos, modifiers);
            }
            auto f32 = meta::reinterpret<float>(u64);
            return p.mem.New<SyntaxFloat>(*pos, modifiers, f32);
        }
        case Suffix::f64: {
            auto f64 = meta::reinterpret<double>(u64);
            return p.mem.New<SyntaxDouble>(*pos, modifiers, f64);
        }
        default: Unreachable();
    }
}

Node Parser::parseDecimal(Node modifiers) {
    auto pos = cur.pos;
    auto val = pos->value();

    cur.advance();

    auto num = readDecimal(val.text, val.end());

    Assert(!num.prefix.length && num.value.length && !num.spec.length);

    auto sfx = readSuffix(num.suffix);
    if (sfx == Suffix::Error) {
        err(pos, "bad decimal suffix: %s#<red>", &num.suffix);
        return empty(pos, modifiers);
    } if (!decimalLengthFitsSuffix(num.value.length, sfx)) {
        err(pos, "%i#<red> decimal digits are too long for suffix %s#<red>", num.value.length, &num.suffix);
        return empty(pos, modifiers);
    }

    auto   len = put(pos, num.value);
    auto   end = numbuf + len;
    char *stop = nullptr;
    _set_errno(0);
    if (isSigned(sfx)) {
        auto i64 = _strtoi64(numbuf, &stop, 10);
        if (errno || stop != end) {
            err(pos, "bad decimal because _strtoi64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return makeInt(*this, pos, modifiers, i64, num, sfx);
    } if (isUnsigned(sfx)) {
        auto u64 = _strtoui64(num.value.text, &stop, 10);
        if (errno) {
            err(pos, "bad decimal because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return makeUInt(*this, pos, modifiers, u64, num, sfx);
    }
    Unreachable();
}

Node Parser::parseHexadecimal(Node modifiers) {
    auto pos = cur.pos;
    auto val = pos->value();

    cur.advance();

    auto num = readHexadecimal(val.text, val.end());

    Assert((num.prefix.length || num.spec.length) && num.value.length);

    auto sfx = readSuffix(num.suffix);
    if (sfx == Suffix::Error) {
        err(pos, "bad hexadecimal suffix: %s#<red>", &num.suffix);
        return empty(pos, modifiers);
    } if (!hexadecimalLengthFitsSuffix(num.value.length, sfx)) {
        err(pos, "%i#<red> hexadecimal digits are too long for suffix %s#<red>", num.value.length, &num.suffix);
        return empty(pos, modifiers);
    }

    auto   len = put(pos, num.value);
    auto   end = numbuf + len;
    char *stop = nullptr;
    _set_errno(0);
    if (isSigned(sfx)) {
        auto i64 = _strtoi64(numbuf, &stop, 16);
        if (errno || stop != end) {
            err(pos, "bad hexadecimal because _strtoi64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return makeInt(*this, pos, modifiers, i64, num, sfx);
    } if (isUnsigned(sfx)) {
        auto u64 = _strtoui64(num.value.text, &stop, 16);
        if (errno) {
            err(pos, "bad hexadecimal because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return makeUInt(*this, pos, modifiers, u64, num, sfx);
    } if (sfx == Suffix::f32) {
        auto u64 = _strtoui64(numbuf, &stop, 16);
        if (errno || stop != end) {
            err(pos, "bad hexadecimal because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return reinterpretFloat(*this, pos, modifiers, u64, num, sfx);
    } if (sfx == Suffix::f64) {
        auto u64 = _strtoui64(numbuf, &stop, 16);
        if (errno || stop != end) {
            err(pos, "bad hexadecimal because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return reinterpretFloat(*this, pos, modifiers, u64, num, sfx);
    }
    Unreachable();
}

Node Parser::parseBinary(Node modifiers) {
    auto pos = cur.pos;
    auto val = pos->value();

    cur.advance();

    auto num = readBinary(val.text, val.end());

    Assert((num.prefix.length || num.spec.length) && num.value.length);

    auto sfx = readSuffix(num.suffix);
    if (sfx == Suffix::Error) {
        err(pos, "bad binary suffix: %s#<red>", &num.suffix);
        return empty(pos, modifiers);
    } if (!binaryLengthFitsSuffix(num.value.length, sfx)) {
        err(pos, "%i#<red> binary digits are too long for suffix %s#<red>", num.value.length, &num.suffix);
        return empty(pos, modifiers);
    }

    auto   len = put(pos, num.value);
    auto   end = numbuf + len;
    char *stop = nullptr;
    _set_errno(0);
    if (isSigned(sfx)) {
        auto i64 = _strtoi64(numbuf, &stop, 2);
        if (errno || stop != end) {
            err(pos, "bad binary because _strtoi64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return makeInt(*this, pos, modifiers, i64, num, sfx);
    } if (isUnsigned(sfx)) {
        auto u64 = _strtoui64(num.value.text, &stop, 2);
        if (errno) {
            err(pos, "bad binary because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return makeUInt(*this, pos, modifiers, u64, num, sfx);
    } if (sfx == Suffix::f32) {
        auto u64 = _strtoui64(numbuf, &stop, 2);
        if (errno || stop != end) {
            err(pos, "bad binary because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return reinterpretFloat(*this, pos, modifiers, u64, num, sfx);
    } if (sfx == Suffix::f64) {
        auto u64 = _strtoui64(numbuf, &stop, 2);
        if (errno || stop != end) {
            err(pos, "bad binary because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return reinterpretFloat(*this, pos, modifiers, u64, num, sfx);
    }
    Unreachable();
}

Node Parser::parseOctal(Node modifiers) {
    auto pos = cur.pos;
    auto val = pos->value();

    cur.advance();

    auto num = readOctal(val.text, val.end());

    Assert((num.prefix.length || num.spec.length) && num.value.length);

    auto sfx = readSuffix(num.suffix);
    if (sfx == Suffix::Error) {
        err(pos, "bad octal suffix: %s#<red>", &num.suffix);
        return empty(pos, modifiers);
    } if (!octalLengthFitsSuffix(num.value.length, sfx)) {
        err(pos, "%i#<red> octal digits are too long for suffix %s#<red>", num.value.length, &num.suffix);
        return empty(pos, modifiers);
    }

    auto   len = put(pos, num.value);
    auto   end = numbuf + len;
    char *stop = nullptr;
    _set_errno(0);
    if (isSigned(sfx)) {
        auto i64 = _strtoi64(numbuf, &stop, 8);
        if (errno || stop != end) {
            err(pos, "bad octal because _strtoi64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return makeInt(*this, pos, modifiers, i64, num, sfx);
    } if (isUnsigned(sfx)) {
        auto u64 = _strtoui64(num.value.text, &stop, 8);
        if (errno) {
            err(pos, "bad octal because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return makeUInt(*this, pos, modifiers, u64, num, sfx);
    } if (sfx == Suffix::f32) {
        auto u64 = _strtoui64(numbuf, &stop, 8);
        if (errno || stop != end) {
            err(pos, "bad octal because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return reinterpretFloat(*this, pos, modifiers, u64, num, sfx);
    } if (sfx == Suffix::f64) {
        auto u64 = _strtoui64(numbuf, &stop, 8);
        if (errno || stop != end) {
            err(pos, "bad octal because _strtoui64 failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return reinterpretFloat(*this, pos, modifiers, u64, num, sfx);
    }
    Unreachable();
}

Node Parser::parseFloat(Node modifiers) {
    auto pos = cur.pos;
    auto val = pos->value();

    cur.advance();

    auto num = readFloat(val.text, val.end());

    Assert(!num.prefix.length && num.value.length && !num.spec.length);

    auto sfx = readSuffix(num.suffix);
    if (sfx == Suffix::Error) {
        err(pos, "bad float suffix: %s#<red>", &num.suffix);
        return empty(pos, modifiers);
    } if (!floatLengthFitsSuffix(num.value.length, sfx)) {
        err(pos, "%i#<red> float digits are too long for suffix %s#<red>", num.value.length, &num.suffix);
        return empty(pos, modifiers);
    }

    auto   len = put(pos, num.value);
    auto   end = numbuf + len;
    char *stop = nullptr;
    _set_errno(0);
    if (sfx == Suffix::None || sfx == Suffix::f64) {
        auto f64 = strtod(numbuf, &stop);
        if (errno || stop != end) {
            err(pos, "bad float because strtod failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return mem.New<SyntaxDouble>(*pos, modifiers, f64);
    } if (sfx == Suffix::f32) {
        auto f32 = strtof(numbuf, &stop);
        if (errno || stop != end) {
            err(pos, "bad float because strtof failed with %cl#<yellow>", numbuf, len);
            return empty(pos, modifiers);
        }
        return mem.New<SyntaxFloat>(*pos, modifiers, f32);
    }
    Unreachable();
}
} // namespace exy