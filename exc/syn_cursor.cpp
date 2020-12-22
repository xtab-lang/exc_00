//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-09
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "parser.h" // Includes syn_cursor.h

#include "source.h"

namespace exy {
Cursor::Cursor(Token pos, Token end) : pos(pos), end(end) {
    Assert(end->kind == Tok::EndOfFile);
}

Token Cursor::advance() {
    prev = pos;
    if (pos == end) {
        return pos;
    }
    Assert(pos < end);
    if (++pos == end) {
        return pos;
    }
    skipWhiteSpace();
    return pos;
}

Token Cursor::peek() {
    auto origPrev = prev;
    auto  origPos = pos;
    auto   peeked = advance();
    pos  = origPos;
    prev = origPrev;
    return peeked;
}

bool Cursor::skipWhiteSpace() {
    auto start = pos;
    while (pos < end) {
        if (pos->kind == Tok::Space || pos->kind == Tok::NewLine) {
            ++pos;
        } else if (pos->kind == Tok::OpenSingleLineComment) {
            skipSingleLineComment();
        } else if (pos->kind == Tok::OpenMultiLineComment) {
            skipMultiLineComment();
        } else {
            break;
        }
    }
    return pos > start;
}

void Cursor::skipSingleLineComment() {
    for (; pos < end && pos->kind != Tok::NewLine; ++pos);
    if (pos < end) {
        ++pos;
    }
}

void Cursor::skipMultiLineComment() {
    auto start = pos;
    for (++pos; pos < end && pos->kind != Tok::CloseMultiLineComment; ++pos);
    if (pos == end) {
        err(start, "unmatched %t", start);
    } else {
        ++pos;
    }
}

void Cursor::rewind(Token _prev, Token _pos) {
    prev = _prev;
    pos = _pos;
}
} // namespace exy