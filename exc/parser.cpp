//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-05
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "parser.h"

#include "syntax.h"
#include "source.h"
#include "compiler.h"
#include "identifiers.h"

namespace exy {
using  Node = SyntaxNode*;
using Nodes = List<SyntaxNode*>&;
using Token = const SourceToken*;

struct Cursor {
    Token pos;
    Token end;

    Cursor(Token pos, Token end) : pos(pos), end(end) {
        Assert(end->kind == Tok::EndOfFile);
    }

    Token advance() {
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

    bool skipWhiteSpace() {
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

    void skipSingleLineComment() {
        for (; pos < end && pos->kind != Tok::NewLine; ++pos);
    }

    void skipMultiLineComment() {
        auto start = pos;
        for (; pos < end && pos->kind != Tok::CloseMultiLineComment; ++pos);
        if (pos == end) {
            err(start, "unmatched %t", start);
        }
    }
};
//------------------------------------------------------------------------------------------------
static bool isaComma(Token pos) { 
    return pos->kind == Tok::Comma;
}
static bool isaDot(Token pos) { 
    return pos->kind == Tok::Dot;
}
static bool isanIdentifier(Token pos) {
    return pos->kind == Tok::Text && pos->keyword == Keyword::None;
}
static bool isanIdentifierOrDot(Token pos) {
    return pos->kind == Tok::Dot || (pos->kind == Tok::Text && pos->keyword == Keyword::None);
}
//------------------------------------------------------------------------------------------------
struct Parser {
    Cursor cur;
    Mem   &mem;

    Parser(const List<SourceToken> &tokens, Mem &mem) : cur(&tokens.first(), &tokens.last()), mem(mem) {}

    void dispose() {
    }

    void parseFile(Nodes nodes) {
        cur.skipWhiteSpace();
        while (cur.pos < cur.end) {
            if (auto node = parseStatement()) {
                nodes.append(node);
            } else {
                Assert(0);
            }
        }
    }

    Node parseStatement() {
        auto pos = cur.pos;
        switch (pos->keyword) {
            case Keyword::Module: return parseModule();
            case Keyword::Import:
            case Keyword::Export: {
                List<Node> nodes{};
                auto node = parseImport(nodes);
                if (nodes.length) {
                    return mem.New<SyntaxCommaList>(*pos, nodes);
                }
                return node;
            }

            default:
                break;
        }
    }
    //--------------------------------------------------------------------------------------------
    // module := 'module' full-name
    Node parseModule() {
        auto node = mem.New<SyntaxModule>(*cur.pos);
        auto  pos = cur.advance(); // Past keyword 'module'.
        if (isanIdentifierOrDot(pos)) {
            node->name = parseFullName();
        } else {
            err(pos, "expected an identifier after 'module' keyword, not %t", pos);
        }
        return node;
    }
    //--------------------------------------------------------------------------------------------
    // import       := 'import' import-list
    // import-list  := import-alias | import-from [',' import-alias | import-from]
    // import-alias := full-name | import-from  ['as' id]
    // import-from  := full-name | import-alias ['from' full-name]
    Node parseImport(List<Node> &list) {
        auto   kw = cur.pos;
        auto node = kw->keyword == Keyword::Import ? (SyntaxImportOrExport*)mem.New<SyntaxImport>(*kw) :
            (SyntaxImportOrExport*)mem.New<SyntaxExport>(*kw);
        auto pos = cur.advance(); // Past 'import' | 'export' keyword.
        if (isanIdentifierOrDot(pos)) {
            parseImport(node);
            while (isaComma(cur.pos)) {
                if (list.isEmpty()) {
                    list.append(node);
                }
                pos = cur.advance(); // Past ','.
                if (isanIdentifierOrDot(pos)) {
                    node = kw->keyword == Keyword::Import ? (SyntaxImportOrExport*)mem.New<SyntaxImport>(*pos) :
                        (SyntaxImportOrExport*)mem.New<SyntaxExport>(*pos);
                    list.append(node);
                    parseImport(node);
                } else {
                    err(pos, "expected an identifier after ',' in '%kw' list, not %t", pos);
                    break;
                }
            }
        } else {
            err(pos, "expected an identifier after 'import' keyword, not %t", pos);
        }
        return node;
    }
    void parseImport(SyntaxImportOrExport *node) {
        node->name = parseFullName();
        if (cur.pos->keyword == Keyword::As) {
            parseImportAlias(node);
        } else if (cur.pos->keyword == Keyword::From) {
            parseImportFrom(node);
        }
    }
    Node parseImportAlias(SyntaxImportOrExport *node) {
        auto  kw = cur.pos;
        auto pos = cur.advance(); // Past 'as' keyword.
        if (isanIdentifier(pos)) {
            node->as = mem.New<SyntaxIdentifier>(*pos, ids.get(pos->value()));
            cur.advance(); // Past id.
        } else {
            err(pos, "expected an identifier after %kw keyword, not %t", kw, pos);
        }
        return node;
    }
    Node parseImportFrom(SyntaxImportOrExport *node) {
        auto &kw = cur.pos;
        auto pos = cur.advance(); // Past 'from' keyword.
        if (isanIdentifierOrDot(pos)) {
            node->from = parseFullName();
            cur.advance(); // Past id.
        } else {
            err(pos, "expected an identifier after %kw keyword, not %t", kw, pos);
        }
        return node;
    }
    //--------------------------------------------------------------------------------------------
    // full-name := id | id ['.' full-name] | '.' id ['.' full-name]
    Node parseFullName() {
        auto start = cur.pos;
        Node node{};
        if (isaDot(start)) {
            // Do nothing. Let lhs be null.
        } else {
            node = (SyntaxNode*)mem.New<SyntaxIdentifier>(*start, ids.get(start->value()));
            cur.advance(); // Past id.
        } while (isaDot(cur.pos)) {
            auto dot = cur.pos;
            auto rhs = cur.advance();
            if (isanIdentifier(rhs)) {
                auto id = mem.New<SyntaxIdentifier>(*rhs, ids.get(rhs->value()));
                node    = mem.New<SyntaxDotExpression>(*start, node, *dot, id);
                cur.advance(); // Past id.
            } else {
                err(rhs, "expected an identifier after '.', not %t", rhs);
                break;
            }
        }
        return node;
    }
};

namespace syn_pass {
void parse(SyntaxFile &file) {
    auto &sourceFile = file.sourceFile();
    traceln("%s#<underline yellow> { file: %s#<underline cyan>, tokens: %i#<magenta>, thread: %i#<green> }",
            sourceFile.path, sourceFile.dotName, sourceFile.tokens.length, GetCurrentThreadId());
    Parser parser{ file.tokens(), comp.syntax->mem };
    parser.parseFile(file.nodes);
    parser.dispose();
}
} // namespace tok_pass
} // namespace exy