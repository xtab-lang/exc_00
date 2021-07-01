#include "pch.h"
#include "parser.h"

#define err(msg, ...) compiler_error("Syntax", cursor.pos, msg, __VA_ARGS__)

namespace exy {
using  Pos = const SourceToken*;
using Node = SyntaxNode*;
//----------------------------------------------------------
auto isDigit(CHAR ch) { return ch >= '0' && ch <= '9'; }
auto isHexDigit(CHAR ch) { return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'); }
auto isBinDigit(CHAR ch) { return ch == '0' || ch == '1'; }
auto isOctDigit(CHAR ch) { return ch >= '0' && ch <= '7'; }
auto isSigned(CHAR ch) { return ch == 'i' || ch == 'I'; }
auto isUnsigned(CHAR ch) { return ch == 'u' || ch == 'U'; }
//----------------------------------------------------------
auto removeUnderscores(const String &v) {
    static const auto bufcap = 0x800;
    static thread_local CHAR buf[bufcap];
    static const auto maxbuflen = bufcap - 1;
    auto buflen = 0;
    for (auto i = 0; i < v.length; ++i) {
        auto ch = v.text[i];
        if (ch == '_') {
            continue;
        }
        if (buflen >= maxbuflen) {
            return String{};
        }
        buf[buflen++] = ch;
    }
    buf[buflen] = '\0';
    return String{ buf, buflen };
}

Node Parser::parseDecimal() {
    auto     v = cursor.pos->sourceValue();
    auto  vpos = v.start();
    auto  vend = v.end();
    auto vmark = vpos;
    const CHAR *vwidth = nullptr;
    Num num{};
    for (; vpos < vend; ++vpos) {
        auto ch = *vpos;
        if (isDigit(ch) || ch == '_') {
            continue;
        }
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits32; // Assume 32 bit integer.
        if (isSigned(ch)) {
            // Do nothing.
        } else if (isUnsigned(ch)) {
            num.isUnsigned = true;
        } else {
            Assert(0);
        }
        if (++vpos == vend) { // Past sufix letter.
            break;
        }
        vwidth = vpos;
        _set_errno(0);
        num.width = Num::Bits(atoi(vpos));
        Assert(errno == 0 && num.width != 0);
        break;
    }
    if (num.width == 0) {
        Assert(num.value.isEmpty() && vpos == vend);
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits32; // Assume 32 bit integer.
    }
    auto node = mem.New<NumberSyntax>(*cursor.pos);
    auto  buf = removeUnderscores(num.value);
    if (buf.isEmpty()) {
        err("decimal number too long: %s#<yellow>", &num.value);
    } else if (num.isUnsigned) {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     u64 = _strtoui64(buf.text, &endPtr, /* radix = */ 10);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad decimal number: %s#<yellow>", &num.value);
        } else if (num.width == Num::Bits8) {
            if (u64 > MAXUINT8) {
                err("decimal value too large for UInt8: %s#<yellow>", &buf);
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt8;
            }
        } else if (num.width == Num::Bits16) {
            if (u64 > MAXUINT16) {
                err("decimal value too large for UInt16: %s#<yellow>", &buf);
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt16;
            }
        } else if (num.width == Num::Bits32) {
            if (u64 > MAXUINT32) {
                if (vwidth == nullptr) {
                    node->u64 = u64;
                    node->type = Keyword::UInt64;
                } else {
                    err("decimal value too large for UInt32: %s#<yellow>", &buf);
                }
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt32;
            }
        } else if (num.width == Num::Bits64) {
            node->u64 = u64;
            node->type = Keyword::UInt64;
        } else {
            Assert(0);
        }
    } else {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     i64 = _strtoi64(buf.text, &endPtr, /* radix = */ 10);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad decimal number: %s#<yellow>", &num.value);
        } else if (num.width == Num::Bits8) {
            if (i64 < MININT8 || i64 > MAXINT8) {
                err("decimal value too large for Int8: %s#<yellow>", &buf);
            } else {
                node->i64 = i64;
                node->type = Keyword::Int8;
            }
        } else if (num.width == Num::Bits16) {
            if (i64 < MININT16 || i64 > MAXINT16) {
                err("decimal value too large for Int16: %s#<yellow>", &buf);
            } else {
                node->i64 = i64;
                node->type = Keyword::Int16;
            }
        } else if (num.width == Num::Bits32) {
            if (i64 < MININT32 || i64 > MAXINT32) {
                if (vwidth == nullptr) {
                    node->i64 = i64;
                    node->type = Keyword::Int64;
                } else {
                    err("decimal value too large for Int32: %s#<yellow>", &buf);
                }
            } else {
                node->i64 = i64;
                node->type = Keyword::Int32;
            }
        } else if (num.width == Num::Bits64) {
            node->i64 = i64;
            node->type = Keyword::Int64;
        } else {
            Assert(0);
        }
    }
    cursor.advance(); // Past number.
    return node;
}

Node Parser::parseHexadecimal() {
    auto     v = cursor.pos->sourceValue();
    auto  vpos = v.start();
    auto  vend = v.end();
    auto vmark = vpos;
    const CHAR *vwidth = nullptr;
    Num num{};
    for (; vpos < vend; ++vpos) {
        auto ch = *vpos;
        if (ch == 'x' || ch == 'X') {
            Assert(vpos - 1 == v.start());
            vmark = vpos + 1;
            continue;
        }
        if (isHexDigit(ch) || ch == '_') {
            continue;
        }
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits32; // Assume 32 bit integer.
        if (ch == 'h' || ch == 'H') {
            if (++vpos == vend) {
                break;
            }
        }
        ch = *vpos;
        if (isSigned(ch)) {
            // Do nothing.
        } else if (isUnsigned(ch)) {
            num.isUnsigned = true;
        } else {
            Assert(0);
        }
        if (++vpos == vend) { // Past sufix letter.
            break;
        }
        vwidth = vpos;
        _set_errno(0);
        num.width = Num::Bits(atoi(vpos));
        Assert(errno == 0 && num.width != 0);
        break;
    }
    if (num.width == 0) {
        Assert(num.value.isEmpty() && vpos == vend);
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits32; // Assume 32 bit integer.
    }
    auto node = mem.New<NumberSyntax>(*cursor.pos);
    auto  buf = removeUnderscores(num.value);
    if (buf.isEmpty()) {
        err("hexadecimal number too long: %s#<yellow>", &num.value);
    } else if (num.isUnsigned) {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     u64 = _strtoui64(buf.text, &endPtr, /* radix = */ 16);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad hexadecimal number: %s#<yellow>", &num.value);
        } else if (num.width == Num::Bits8) {
            if (u64 > MAXUINT8) {
                err("hexadecimal value too large for UInt8: %s#<yellow>", &buf);
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt8;
            }
        } else if (num.width == Num::Bits16) {
            if (u64 > MAXUINT16) {
                err("hexadecimal value too large for UInt16: %s#<yellow>", &buf);
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt16;
            }
        } else if (num.width == Num::Bits32) {
            if (u64 > MAXUINT32) {
                if (vwidth == nullptr) {
                    node->u64 = u64;
                    node->type = Keyword::UInt64;
                } else {
                    err("hexadecimal value too large for UInt32: %s#<yellow>", &buf);
                }
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt32;
            }
        } else if (num.width == Num::Bits64) {
            node->u64 = u64;
            node->type = Keyword::UInt64;
        } else {
            Assert(0);
        }
    } else {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     i64 = _strtoi64(buf.text, &endPtr, /* radix = */ 16);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad hexadecimal number: %s#<yellow>", &num.value);
        } else if (num.width == Num::Bits8) {
            if (i64 < MININT8 || i64 > MAXINT8) {
                err("hexadecimal value too large for Int8: %s#<yellow>", &buf);
            } else {
                node->i64 = i64;
                node->type = Keyword::Int8;
            }
        } else if (num.width == Num::Bits16) {
            if (i64 < MININT16 || i64 > MAXINT16) {
                err("hexadecimal value too large for Int16: %s#<yellow>", &buf);
            } else {
                node->i64 = i64;
                node->type = Keyword::Int16;
            }
        } else if (num.width == Num::Bits32) {
            if (i64 < MININT32 || i64 > MAXINT32) {
                if (vwidth == nullptr) {
                    node->i64 = i64;
                    node->type = Keyword::Int64;
                } else {
                    err("hexadecimal value too large for Int32: %s#<yellow>", &buf);
                }
            } else {
                node->i64 = i64;
                node->type = Keyword::Int32;
            }
        } else if (num.width == Num::Bits64) {
            node->i64 = i64;
            node->type = Keyword::Int64;
        } else {
            Assert(0);
        }
    }
    cursor.advance(); // Past number.
    return node;
}

Node Parser::parseBinary() {
    auto     v = cursor.pos->sourceValue();
    auto  vpos = v.start();
    auto  vend = v.end();
    auto vmark = vpos;
    const CHAR *vwidth = nullptr;
    Num num{};
    for (; vpos < vend; ++vpos) {
        auto ch = *vpos;
        if (ch == 'b' || ch == 'B') {
            Assert(vpos - 1 == v.start());
            vmark = vpos + 1;
            continue;
        }
        if (isBinDigit(ch) || ch == '_') {
            continue;
        }
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits32; // Assume 32 bit integer.
        if (ch == 'b' || ch == 'B') {
            if (++vpos == vend) {
                break;
            }
        }
        ch = *vpos;
        if (isSigned(ch)) {
            // Do nothing.
        } else if (isUnsigned(ch)) {
            num.isUnsigned = true;
        } else {
            Assert(0);
        }
        if (++vpos == vend) { // Past sufix letter.
            break;
        }
        vwidth = vpos;
        _set_errno(0);
        num.width = Num::Bits(atoi(vpos));
        Assert(errno == 0 && num.width != 0);
        break;
    }
    if (num.width == 0) {
        Assert(num.value.isEmpty() && vpos == vend);
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits32; // Assume 32 bit integer.
    }
    auto node = mem.New<NumberSyntax>(*cursor.pos);
    auto  buf = removeUnderscores(num.value);
    if (buf.isEmpty()) {
        err("binary number too long: %s#<yellow>", &num.value);
    } else if (num.isUnsigned) {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     u64 = _strtoui64(buf.text, &endPtr, /* radix = */ 2);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad binary number: %s#<yellow>", &num.value);
        } else if (num.width == Num::Bits8) {
            if (u64 > MAXUINT8) {
                err("binary value too large for UInt8: %s#<yellow>", &buf);
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt8;
            }
        } else if (num.width == Num::Bits16) {
            if (u64 > MAXUINT16) {
                err("binary value too large for UInt16: %s#<yellow>", &buf);
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt16;
            }
        } else if (num.width == Num::Bits32) {
            if (u64 > MAXUINT32) {
                if (vwidth == nullptr) {
                    node->u64 = u64;
                    node->type = Keyword::UInt64;
                } else {
                    err("binary value too large for UInt32: %s#<yellow>", &buf);
                }
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt32;
            }
        } else if (num.width == Num::Bits64) {
            node->u64 = u64;
            node->type = Keyword::UInt64;
        } else {
            Assert(0);
        }
    } else {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     i64 = _strtoi64(buf.text, &endPtr, /* radix = */ 2);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad binary number: %s#<yellow>", &num.value);
        } else if (num.width == Num::Bits8) {
            if (i64 < MININT8 || i64 > MAXINT8) {
                err("binary value too large for Int8: %s#<yellow>", &buf);
            } else {
                node->i64 = i64;
                node->type = Keyword::Int8;
            }
        } else if (num.width == Num::Bits16) {
            if (i64 < MININT16 || i64 > MAXINT16) {
                err("binary value too large for Int16: %s#<yellow>", &buf);
            } else {
                node->i64 = i64;
                node->type = Keyword::Int16;
            }
        } else if (num.width == Num::Bits32) {
            if (i64 < MININT32 || i64 > MAXINT32) {
                if (vwidth == nullptr) {
                    node->i64 = i64;
                    node->type = Keyword::Int64;
                } else {
                    err("binary value too large for Int32: %s#<yellow>", &buf);
                }
            } else {
                node->i64 = i64;
                node->type = Keyword::Int32;
            }
        } else if (num.width == Num::Bits64) {
            node->i64 = i64;
            node->type = Keyword::Int64;
        } else {
            Assert(0);
        }
    }
    cursor.advance(); // Past number.
    return node;
}

Node Parser::parseOctal() {
    auto     v = cursor.pos->sourceValue();
    auto  vpos = v.start();
    auto  vend = v.end();
    auto vmark = vpos;
    const CHAR *vwidth = nullptr;
    Num num{};
    for (; vpos < vend; ++vpos) {
        auto ch = *vpos;
        if (ch == 'o' || ch == 'O') {
            Assert(vpos - 1 == v.start());
            vmark = vpos + 1;
            continue;
        }
        if (isOctDigit(ch) || ch == '_') {
            continue;
        }
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits32; // Assume 32 bit integer.
        if (ch == 'o' || ch == 'O') {
            if (++vpos == vend) {
                break;
            }
        }
        ch = *vpos;
        if (isSigned(ch)) {
            // Do nothing.
        } else if (isUnsigned(ch)) {
            num.isUnsigned = true;
        } else {
            Assert(0);
        }
        if (++vpos == vend) { // Past sufix letter.
            break;
        }
        vwidth = vpos;
        _set_errno(0);
        num.width = Num::Bits(atoi(vpos));
        Assert(errno == 0 && num.width != 0);
        break;
    }
    if (num.width == 0) {
        Assert(num.value.isEmpty() && vpos == vend);
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits32; // Assume 32 bit integer.
    }
    auto node = mem.New<NumberSyntax>(*cursor.pos);
    auto  buf = removeUnderscores(num.value);
    if (buf.isEmpty()) {
        err("octal number too long: %s#<yellow>", &num.value);
    } else if (num.isUnsigned) {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto u64 = _strtoui64(buf.text, &endPtr, /* radix = */ 8);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad octal number: %s#<yellow>", &num.value);
        } else if (num.width == Num::Bits8) {
            if (u64 > MAXUINT8) {
                err("octal value too large for UInt8: %s#<yellow>", &buf);
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt8;
            }
        } else if (num.width == Num::Bits16) {
            if (u64 > MAXUINT16) {
                err("octal value too large for UInt16: %s#<yellow>", &buf);
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt16;
            }
        } else if (num.width == Num::Bits32) {
            if (u64 > MAXUINT32) {
                if (vwidth == nullptr) {
                    node->u64 = u64;
                    node->type = Keyword::UInt64;
                } else {
                    err("octal value too large for UInt32: %s#<yellow>", &buf);
                }
            } else {
                node->u64 = u64;
                node->type = Keyword::UInt32;
            }
        } else if (num.width == Num::Bits64) {
            node->u64 = u64;
            node->type = Keyword::UInt64;
        } else {
            Assert(0);
        }
    } else {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     i64 = _strtoi64(buf.text, &endPtr, /* radix = */ 8);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad octal number: %s#<yellow>", &num.value);
        } else if (num.width == Num::Bits8) {
            if (i64 < MININT8 || i64 > MAXINT8) {
                err("octal value too large for Int8: %s#<yellow>", &buf);
            } else {
                node->i64 = i64;
                node->type = Keyword::Int8;
            }
        } else if (num.width == Num::Bits16) {
            if (i64 < MININT16 || i64 > MAXINT16) {
                err("octal value too large for Int16: %s#<yellow>", &buf);
            } else {
                node->i64 = i64;
                node->type = Keyword::Int16;
            }
        } else if (num.width == Num::Bits32) {
            if (i64 < MININT32 || i64 > MAXINT32) {
                if (vwidth == nullptr) {
                    node->i64 = i64;
                    node->type = Keyword::Int64;
                } else {
                    err("octal value too large for Int32: %s#<yellow>", &buf);
                }
            } else {
                node->i64 = i64;
                node->type = Keyword::Int32;
            }
        } else if (num.width == Num::Bits64) {
            node->i64 = i64;
            node->type = Keyword::Int64;
        } else {
            Assert(0);
        }
    }
    cursor.advance(); // Past number.
    return node;
}

Node Parser::parseFloat() {
    auto     v = cursor.pos->sourceValue();
    auto  vpos = v.start();
    auto  vend = v.end();
    auto vmark = vpos;
    Num num{};
    for (; vpos < vend; ++vpos) {
        auto ch = *vpos;
        if (isDigit(ch) || ch == '_' || ch == '.' || ch == 'e' || ch == 'E' || ch == '-' || ch == '+') {
            continue;
        }
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
        if (ch == 'f' || ch == 'F') {
            num.width = Num::Bits32;
        } else {
            Assert(0);
        }
        if (++vpos == vend) { // Past sufix letter.
            break;
        }
        _set_errno(0);
        num.width = Num::Bits(atoi(vpos));
        Assert(errno == 0 && num.width != 0);
        break;
    }
    if (num.width == 0) {
        Assert(num.value.isEmpty() && vpos == vend);
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
    }
    auto node = mem.New<NumberSyntax>(*cursor.pos);
    auto  buf = removeUnderscores(num.value);
    if (buf.isEmpty()) {
        err("floating-point number too long: %s#<yellow>", &num.value);
    } else if (num.width == Num::Bits32) {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     f32 = strtof(buf.text, &endPtr);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad f32: %s#<yellow>", &num.value);
        } else {
            node->f32 = f32;
            node->type = Keyword::Float;
        }
    } else if (num.width == Num::Bits64) {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     f64 = strtod(buf.text, &endPtr);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad f64: %s#<yellow>", &num.value);
        } else {
            node->f64 = f64;
            node->type = Keyword::Double;
        }
    } else {
        Assert(0);
    }
    cursor.advance(); // Past number.
    return node;
}

Node Parser::parseDecimalFloat() {
    auto     v = cursor.pos->sourceValue();
    auto  vpos = v.start();
    auto  vend = v.end();
    auto vmark = vpos;
    const CHAR *vwidth = nullptr;
    Num num{};
    for (; vpos < vend; ++vpos) {
        auto ch = *vpos;
        if (isDigit(ch) || ch == '_') {
            continue;
        }
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
        if (ch == 'f' || ch == 'F') {
            num.width = Num::Bits32;
        } else {
            Assert(0);
        }
        if (++vpos == vend) { // Past sufix letter.
            break;
        }
        vwidth = vpos;
        _set_errno(0);
        num.width = Num::Bits(atoi(vpos));
        Assert(errno == 0 && num.width != 0);
        break;
    }
    if (num.width == 0) {
        Assert(num.value.isEmpty() && vpos == vend);
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
    }
    auto node = mem.New<NumberSyntax>(*cursor.pos);
    auto  buf = removeUnderscores(num.value);
    if (buf.isEmpty()) {
        err("decimal number too long: %s#<yellow>", &num.value);
    } else {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     u64 = _strtoui64(buf.text, &endPtr, /* radix = */ 10);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad decimal number: %s#<yellow>", &num.value);
        } else {
            switch (num.width) {
                case Num::Bits64: {
                    node->f64 = meta::reinterpret<DOUBLE>(u64);
                    node->type = Keyword::Double;
                } break;
                case Num::Bits32: if (u64 > MAXUINT32) {
                    if (vwidth == nullptr) {
                        node->f64 = meta::reinterpret<DOUBLE>(u64);
                        node->type = Keyword::Double;
                    } else {
                        err("decimal bit value too large for f32: %s#<yellow>", &buf);
                    }
                } else {
                    node->f32 = meta::reinterpret<FLOAT>(UINT32(u64));
                    node->type = Keyword::Float;
                } break;
                default:
                    Assert(0);
                    break;
            }
        }
    }
    cursor.advance(); // Past number.
    return node;
}

Node Parser::parseHexadecimalFloat() {
    auto     v = cursor.pos->sourceValue();
    auto  vpos = v.start();
    auto  vend = v.end();
    auto vmark = vpos;
    const CHAR *vwidth = nullptr;
    Num num{};
    for (; vpos < vend; ++vpos) {
        auto ch = *vpos;
        if (ch == 'x' || ch == 'X') {
            Assert(vpos - 1 == v.start());
            vmark = vpos + 1;
            continue;
        }
        if (isHexDigit(ch) || ch == '_') {
            continue;
        }
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
        if (ch == 'h' || ch == 'H') {
            ++vpos;
            Assert(vpos < vend);
        }
        ch = *vpos;
        if (ch == 'p' || ch == 'P') {
            num.width = Num::Bits32;
        } else {
            Assert(0);
        }
        if (++vpos == vend) { // Past sufix letter.
            break;
        }
        vwidth = vpos;
        _set_errno(0);
        num.width = Num::Bits(atoi(vpos));
        Assert(errno == 0 && num.width != 0);
        break;
    }
    if (num.width == 0) {
        Assert(num.value.isEmpty() && vpos == vend);
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
    }
    auto node = mem.New<NumberSyntax>(*cursor.pos);
    auto  buf = removeUnderscores(num.value);
    if (buf.isEmpty()) {
        err("hexadecimal number too long: %s#<yellow>", &num.value);
    } else {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     u64 = _strtoui64(buf.text, &endPtr, /* radix = */ 16);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad hexadecimal number: %s#<yellow>", &num.value);
        } else {
            switch (num.width) {
                case Num::Bits64: {
                    node->f64 = meta::reinterpret<DOUBLE>(u64);
                    node->type = Keyword::Double;
                } break;
                case Num::Bits32: if (u64 > MAXUINT32) {
                    if (vwidth == nullptr) {
                        node->f64 = meta::reinterpret<DOUBLE>(u64);
                        node->type = Keyword::Double;
                    } else {
                        err("hexadecimal bit value too large for f32: %s#<yellow>", &buf);
                    }
                } else {
                    node->f32 = meta::reinterpret<FLOAT>(UINT32(u64));
                    node->type = Keyword::Float;
                } break;
                default:
                    Assert(0);
                    break;
            }
        }
    }
    cursor.advance(); // Past number.
    return node;
}

Node Parser::parseBinaryFloat() {
    auto     v = cursor.pos->sourceValue();
    auto  vpos = v.start();
    auto  vend = v.end();
    auto vmark = vpos;
    const CHAR *vwidth = nullptr;
    Num num{};
    for (; vpos < vend; ++vpos) {
        auto ch = *vpos;
        if (ch == 'b' || ch == 'B') {
            Assert(vpos - 1 == v.start());
            vmark = vpos + 1;
            continue;
        }
        if (isBinDigit(ch) || ch == '_') {
            continue;
        }
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
        if (ch == 'b' || ch == 'B') {
            ++vpos;
            Assert(vpos < vend);
        }
        ch = *vpos;
        if (ch == 'f' || ch == 'F') {
            num.width = Num::Bits32;
        } else {
            Assert(0);
        }
        if (++vpos == vend) { // Past sufix letter.
            break;
        }
        vwidth = vpos;
        _set_errno(0);
        num.width = Num::Bits(atoi(vpos));
        Assert(errno == 0 && num.width != 0);
        break;
    }
    if (num.width == 0) {
        Assert(num.value.isEmpty() && vpos == vend);
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
    }
    auto node = mem.New<NumberSyntax>(*cursor.pos);
    auto  buf = removeUnderscores(num.value);
    if (buf.isEmpty()) {
        err("binary number too long: %s#<yellow>", &num.value);
    } else {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     u64 = _strtoui64(buf.text, &endPtr, /* radix = */ 2);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad binary number: %s#<yellow>", &num.value);
        } else {
            switch (num.width) {
                case Num::Bits64: {
                    node->f64 = meta::reinterpret<DOUBLE>(u64);
                    node->type = Keyword::Double;
                } break;
                case Num::Bits32: if (u64 > MAXUINT32) {
                    if (vwidth == nullptr) {
                        node->f64 = meta::reinterpret<DOUBLE>(u64);
                        node->type = Keyword::Double;
                    } else {
                        err("binary bit value too large for f32: %s#<yellow>", &buf);
                    }
                } else {
                    node->f32 = meta::reinterpret<FLOAT>(UINT32(u64));
                    node->type = Keyword::Float;
                } break;
                default:
                    Assert(0);
                    break;
            }
        }
    }
    cursor.advance(); // Past number.
    return node;
}

Node Parser::parseOctalFloat() {
    auto     v = cursor.pos->sourceValue();
    auto  vpos = v.start();
    auto  vend = v.end();
    auto vmark = vpos;
    const CHAR *vwidth = nullptr;
    Num num{};
    for (; vpos < vend; ++vpos) {
        auto ch = *vpos;
        if (ch == 'o' || ch == 'O') {
            Assert(vpos - 1 == v.start());
            vmark = vpos + 1;
            continue;
        }
        if (isOctDigit(ch) || ch == '_') {
            continue;
        }
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
        if (ch == 'o' || ch == 'O') {
            ++vpos;
            Assert(vpos < vend);
        }
        ch = *vpos;
        if (ch == 'f' || ch == 'F') {
            num.width = Num::Bits32;
        } else {
            Assert(0);
        }
        if (++vpos == vend) { // Past sufix letter.
            break;
        }
        vwidth = vpos;
        _set_errno(0);
        num.width = Num::Bits(atoi(vpos));
        Assert(errno == 0 && num.width != 0);
        break;
    }
    if (num.width == 0) {
        Assert(num.value.isEmpty() && vpos == vend);
        num.value = { vmark, INT(vpos - vmark) };
        num.width = Num::Bits64; // Assume f64.
    }
    auto node = mem.New<NumberSyntax>(*cursor.pos);
    auto  buf = removeUnderscores(num.value);
    if (buf.isEmpty()) {
        err("octal number too long: %s#<yellow>", &num.value);
    } else {
        _set_errno(0);
        CHAR *endPtr = nullptr;
        auto     u64 = _strtoui64(buf.text, &endPtr, /* radix = */ 8);
        if (errno != 0 || endPtr != buf.end()) {
            err("bad octal number: %s#<yellow>", &num.value);
        } else {
            switch (num.width) {
                case Num::Bits64: {
                    node->f64 = meta::reinterpret<DOUBLE>(u64);
                    node->type = Keyword::Double;
                } break;
                case Num::Bits32: if (u64 > MAXUINT32) {
                    if (vwidth == nullptr) {
                        node->f64 = meta::reinterpret<DOUBLE>(u64);
                        node->type = Keyword::Double;
                    } else {
                        err("octal bit value too large for f32: %s#<yellow>", &buf);
                    }
                } else {
                    node->f32 = meta::reinterpret<FLOAT>(UINT32(u64));
                    node->type = Keyword::Float;
                } break;
                default:
                    Assert(0);
                    break;
            }
        }
    }
    cursor.advance(); // Past number.
    return node;
}
} // namespace exy