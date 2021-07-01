#include "pch.h"
#include "parser.h"

#define err(pos, msg, ...) compiler_error("Syntax", pos, msg, __VA_ARGS__)

namespace exy {
using  Pos = const SourceToken*;
using Node = SyntaxNode*;

static Pos findOpenInterpolationTokenOrEOS(Parser::Cursor &cursor, Tok close) {
    Pos prev = nullptr;
    while (cursor.pos->kind != Tok::EndOfFile) {
        if (cursor.pos->kind == close) {
            if (prev == nullptr || prev->kind != Tok::BackSlash) {
                return cursor.pos;
            }
        } else if (cursor.pos->kind == Tok::HashOpenParen || cursor.pos->kind == Tok::HashOpenBracket) {
            if (prev == nullptr || prev->kind != Tok::BackSlash) {
                return cursor.pos;
            }
        }
        prev = cursor.pos;
        cursor.advance();
    }
    return nullptr;
}

static auto getCloseOf(auto pos) {
    switch (pos->kind) {
        case Tok::SingleQuote:
        case Tok::WideSingleQuote:
        case Tok::RawSingleQuote:
            return Tok::SingleQuote;
        case Tok::DoubleQuote:
        case Tok::WideDoubleQuote:
        case Tok::RawDoubleQuote:
            return Tok::DoubleQuote;
    }
    UNREACHABLE();
}

static auto isDoubleQuote(auto pos) {
    switch (pos->kind) {
        case Tok::DoubleQuote:
        case Tok::WideDoubleQuote:
        case Tok::RawDoubleQuote:
            return true;
    }
    return false;
}

Node Parser::parseQuoted() {
    Node node = nullptr;
    auto open = cursor.pos;
    cursor.advance(); // Past open-quote.
    auto mark = cursor.pos;
    auto close = getCloseOf(open);
    while (true) {
        auto found = findOpenInterpolationTokenOrEOS(cursor, close);
        if (found == nullptr) {
            // Did not find '#(' | '#[' | close-quote before EOF.
            err(open, "unmatched %tok", open);
            String value{ mark->pos.range.start.text, cursor.end->pos.range.start.text };
            if (node == nullptr) {
                node = mem.New<TextSyntax>(*open, value, *cursor.pos);
            }
            break;
        }
        if (found->kind == Tok::HashOpenParen || found->kind == Tok::HashOpenBracket) {
            // Found '#(' | '#['.
            String value{ mark->pos.range.start.text, found->pos.range.start.text };
            if (value.isNotEmpty()) {
                auto text = mem.New<TextSyntax>(*mark, value, *found);
                if (auto interpolation = (InterpolationSyntax*)node) {
                    interpolation->nodes.append(text);
                } else {
                    Assert(node == nullptr);
                    interpolation = mem.New<InterpolationSyntax>(*open, text);
                    node = interpolation;
                }
            }
            if (auto interpolation = (InterpolationSyntax*)node) {
                interpolation->nodes.append(parseCodeBlock());
            } else {
                Assert(node == nullptr);
                interpolation = mem.New<InterpolationSyntax>(*open, parseCodeBlock());
                node = interpolation;
            }
            mark = cursor.pos;
            if (mark == cursor.end) {
                err(open, "unmatched %tok", open);
                break;
            }
        } else { // Found close-quote.
            String value{ mark->pos.range.start.text, found->pos.range.start.text };
            if (auto interpolation = (InterpolationSyntax*)node) {
                if (value.isNotEmpty()) {
                    auto text = mem.New<TextSyntax>(*mark, value, *found);
                    interpolation->nodes.append(text);
                }
                interpolation->close = cursor.pos;
            } else if (isDoubleQuote(open)) {
                node = mem.New<DoubleQuotedSyntax>(*open, value, *found);
            } else {
                node = mem.New<SingleQuotedSyntax>(*open, value, *found);
            }
            cursor.advance(); // Past close-quote.
            break;
        }
    }
    return node;
}
} // namespace exy