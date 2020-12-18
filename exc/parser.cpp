//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-05
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "parser.h"

#include "source.h"

namespace exy {
//------------------------------------------------------------------------------------------------
static bool isOpenParen(Token pos) {
    return pos->kind == Tok::OpenParen;
}
static bool isCloseParen(Token pos) {
    return pos->kind == Tok::CloseParen;
}
static bool isOpenBracket(Token pos) {
    return pos->kind == Tok::OpenBracket;
}
static bool isCloseBracket(Token pos) {
    return pos->kind == Tok::CloseBracket;
}
static bool isOpenAngle(Token pos) {
    return pos->kind == Tok::OpenAngle;
}
static bool isCloseAngle(Token pos) {
    return pos->kind == Tok::CloseAngle;
}
static bool isOpenCurly(Token pos) {
    return pos->kind == Tok::OpenCurly;
}
static bool isCloseCurly(Token pos) {
    return pos->kind == Tok::CloseCurly;
}
static bool isAssign(Token pos) {
    return pos->kind == Tok::Assign;
}
static bool isaModifier(Token pos) {
    return pos->keyword > Keyword::_begin_modifiers && pos->keyword < Keyword::_end_modifiers;
}
static bool isaColon(Token pos) {
    return pos->kind == Tok::Colon;
}
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
static bool isaNewLine(Token pos) {
    return pos->kind == Tok::NewLine;
}
//------------------------------------------------------------------------------------------------
Parser::Parser(const List<SourceToken> &tokens, Mem &mem) : cur(&tokens.first(), &tokens.last()), mem(mem) {}

void Parser::dispose() {}

void Parser::parseFile(SyntaxFile &file) {
    cur.skipWhiteSpace();
    while (cur.pos < cur.end) {
        if (auto node = parseStatement()) {
            if (node->kind == SyntaxKind::Module) {
                setModule(file, (SyntaxModule*)node);
            } else {
                file.nodes.append(node);
            }
        } else {
            cur.advance();
        }
    } if (!file.mod) {
        createModule(file);
    }
}

void Parser::setModule(SyntaxFile &file, SyntaxModule *mod) {
    if (file.mod) {
        err(mod, "module declaration already seen at %pos", file.mod->pos);
        ndispose(mod);
        return;
    }
    file.mod = mod;
}

void Parser::createModule(SyntaxFile &file) {
    auto &pos = file.tokens().first();
    auto node = mem.New<SyntaxModule>(pos, nullptr);
    auto name = mem.New<SyntaxIdentifier>(pos, nullptr, ids.get(file.sourceFile().name));
    node->name = mem.New<SyntaxDotExpression>(pos, nullptr, nullptr, pos, name);
    setModule(file, node);
}

Node Parser::parseStatement() {
    auto modifiers = parseModifiers();
    auto pos = cur.pos;
    switch (pos->keyword) {
        case Keyword::Module: return parseModule(modifiers);

        case Keyword::Import:
        case Keyword::Export: {
            List<Node> nodes{};
            auto node = parseImport(nodes);
            if (nodes.length) {
                return mem.New<SyntaxCommaList>(*pos, modifiers, nodes);
            }
            node->modifiers = modifiers;
            return node;
        }

        case Keyword::Let: return parseLet(modifiers);

    }
    return parseExpression(modifiers);
}
//------------------------------------------------------------------------------------------------
// module := 'module' full-name
Node Parser::parseModule(Node modifiers) {
    auto node = mem.New<SyntaxModule>(*cur.pos, modifiers);
    auto  pos = cur.advance(); // Past keyword 'module'.
    if (isanIdentifierOrDot(pos)) {
        node->name = parseFullName(nullptr);
    } else {
        err(pos, "expected an identifier after 'module' keyword, not %t", pos);
    }
    return node;
}
//------------------------------------------------------------------------------------------------
// import       := 'import' import-list
// import-list  := import-alias | import-from [',' import-alias | import-from]
// import-alias := full-name | import-from  ['as' id]
// import-from  := full-name | import-alias ['from' full-name]
Node Parser::parseImport(Nodes list) {
    auto   kw = cur.pos;
    auto node = kw->keyword == Keyword::Import ? (SyntaxImportOrExport*)mem.New<SyntaxImport>(*kw) :
        (SyntaxImportOrExport*)mem.New<SyntaxExport>(*kw);
    auto pos = cur.advance(); // Past 'import' | 'export' keyword.
    if (isanIdentifierOrDot(pos)) {
        parseImport(node);
        while (isaComma(cur.pos)) {
            auto comma = cur.pos;
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
                err(pos, "expected an identifier after %t in %t list, not %t", comma, kw, pos);
                break;
            }
        }
    } else {
        err(pos, "expected an identifier after 'import' keyword, not %t", pos);
    }
    return node;
}
void Parser::parseImport(SyntaxImportOrExport *node) {
    node->name = parseFullName(nullptr);
    if (cur.pos->keyword == Keyword::As) {
        parseImportAlias(node);
        if (cur.pos->keyword == Keyword::From) {
            parseImportFrom(node);
        }
    } else if (cur.pos->keyword == Keyword::From) {
        parseImportFrom(node);
        if (cur.pos->keyword == Keyword::As) {
            parseImportAlias(node);
        }
    }
}
Node Parser::parseImportAlias(SyntaxImportOrExport *node) {
    auto  kw = cur.pos;
    auto pos = cur.advance(); // Past 'as' keyword.
    if (isanIdentifier(pos)) {
        node->as = mem.New<SyntaxIdentifier>(*pos, nullptr, ids.get(pos->value()));
        cur.advance(); // Past id.
    } else {
        err(pos, "expected an identifier after %t keyword, not %t", kw, pos);
    }
    return node;
}
Node Parser::parseImportFrom(SyntaxImportOrExport *node) {
    auto  kw = cur.pos;
    auto pos = cur.advance(); // Past 'from' keyword.
    if (isanIdentifierOrDot(pos)) {
        node->from = parseFullName(nullptr);
    } else {
        err(pos, "expected an identifier after %t keyword, not %t", kw, pos);
    }
    return node;
}
//------------------------------------------------------------------------------------------------
// let := 'let' id '=' expr
Node Parser::parseLet(Node modifiers) {
    auto   kw = cur.pos;
    auto node = mem.New<SyntaxLet>(*kw, modifiers);
    cur.advance(); // Past 'let' keyword.
    if (isanIdentifier(cur.pos)) {
        auto    id = cur.pos;
        node->name = mem.New<SyntaxIdentifier>(*id, nullptr, ids.get(id->value()), id->keyword);
        cur.advance(); // Past {id}.
        if (isAssign(cur.pos)) {
            cur.advance(); // Past '='.
            node->value = parseExpression();
        } else if (isaComma(cur.pos)) {
            auto comma = cur.pos;
            auto  list = mem.New<SyntaxCommaList>(node->name->pos, nullptr);
            list->nodes.append(node->name);
            node->name = list;
            cur.advance(); // Past ','.
            if (!isanIdentifier(cur.pos)) {
                err(cur.pos, "expected an identifier after %t in let statement, not %t", comma, cur.pos);
            } else while (true) {
                id = cur.pos;
                list->nodes.append(mem.New<SyntaxIdentifier>(*id, nullptr, ids.get(id->value()), id->keyword));
                cur.advance(); // Past {id}.
                if (isaComma(cur.pos)) {
                    comma = cur.pos;
                    cur.advance();
                    if (!isanIdentifier(cur.pos)) {
                        err(cur.pos, "expected an identifier after %t in let statement, not %t", comma, cur.pos);
                        break;
                    }
                } else {
                    break;
                }
            } if (isAssign(cur.pos)) {
                cur.advance(); // Past '='.
                node->value = parseExpression();
            } else {
                err(cur.pos, "expected %tv in let statement, not %t", Tok::Assign, cur.pos);
            }
        } else {
            err(cur.pos, "expected %tv or %tv after %t, not %t", Tok::Assign, Tok::Comma, id, 
                cur.pos);
        }
    } else {
        err(cur.pos, "expected an identifier or comma after %t keyword, not %t", kw, cur.pos);
    }
    return node;
}
//------------------------------------------------------------------------------------------------
// expression := unary-prefix
Node Parser::parseExpression() {
    return parseExpression(parseModifiers());
}
Node Parser::parseExpression(Node modifiers) {
    if (auto term = parseUnaryPrefix(modifiers)) {
        return term;
    } if (modifiers) {
        err(modifiers, "stray modifiers");
        ndispose(modifiers);
    }
    return nullptr;
}
//------------------------------------------------------------------------------------------------
// unary-prefix := unary-suffix
//            or   '-' | '+' | '--' | '++' | '*' | '&' | '!' unary-prefix
Node Parser::parseUnaryPrefix(Node modifiers) {
    auto pos = cur.pos;
    switch (pos->kind) {
        case Tok::Minus: {
            ((SourceToken*)pos)->kind = Tok::UnaryMinus;
            auto node = mem.New<SyntaxUnaryPrefix>(*pos, modifiers, *pos);
            cur.advance();
            node->value = parseUnaryPrefix(nullptr);
            return node;
        }
        case Tok::Plus: {
            ((SourceToken*)pos)->kind = Tok::UnaryPlus;
            auto node = mem.New<SyntaxUnaryPrefix>(*pos, modifiers, *pos);
            cur.advance();
            node->value = parseUnaryPrefix(nullptr);
            return node;
        }
        case Tok::MinusMinus:
        case Tok::PlusPlus: {
            auto node = mem.New<SyntaxUnaryPrefix>(*pos, modifiers, *pos);
            cur.advance();
            node->value = parseUnaryPrefix(nullptr);
            return node;
        }
        case Tok::Multiply: {
            ((SourceToken*)pos)->kind = Tok::Dereference;
            auto node = mem.New<SyntaxUnaryPrefix>(*pos, modifiers, *pos);
            cur.advance();
            node->value = parseUnaryPrefix(nullptr);
            return node;
        }
        case Tok::And: {
            ((SourceToken*)pos)->kind = Tok::AddressOf;
            auto node = mem.New<SyntaxUnaryPrefix>(*pos, modifiers, *pos);
            cur.advance();
            node->value = parseUnaryPrefix(nullptr);
            return node;
        }
        case Tok::Exponentiation: {
            ((SourceToken*)pos)->kind = Tok::Dereference;
            auto  node = mem.New<SyntaxUnaryPrefix>(*pos, modifiers, *pos);
            auto inner = mem.New<SyntaxUnaryPrefix>(*pos, nullptr, *pos);
            node->value = inner;
            cur.advance();
            inner->value = parseUnaryPrefix(nullptr);
            return node;
        }
        case Tok::AndAnd: {
            ((SourceToken*)pos)->kind = Tok::AddressOf;
            auto  node = mem.New<SyntaxUnaryPrefix>(*pos, modifiers, *pos);
            auto inner = mem.New<SyntaxUnaryPrefix>(*pos, nullptr, *pos);
            node->value = inner;
            cur.advance();
            inner->value = parseUnaryPrefix(nullptr);
            return node;
        }
        case Tok::LogicalNot: {
            auto node = mem.New<SyntaxUnaryPrefix>(*pos, modifiers, *pos);
            cur.advance();
            node->value = parseUnaryPrefix(nullptr);
            return node;
        }
    }
    return parseUnarySuffix(modifiers);
}
//------------------------------------------------------------------------------------------------
// unary-suffix := dot-expression
//            or   unary-suffix '--' | '++' | '*' | '&'
Node Parser::parseUnarySuffix(Node modifiers) {
    if (auto node = parseDotExpression(modifiers)) {
        auto   pos = cur.pos;
        auto moved = false;
        do {
            moved = false;
            switch (pos->kind) {
                case Tok::MinusMinus:
                case Tok::PlusPlus:
                case Tok::Pointer:
                case Tok::Reference: {
                    auto unary = mem.New<SyntaxUnarySuffix>(*pos, modifiers, node, *pos);
                    node->modifiers = nullptr;
                    node = unary;
                    cur.advance();
                    moved = true;
                } break;
            }
        } while (moved);
        return node;
    }
    return nullptr;
}
//------------------------------------------------------------------------------------------------
// dot-expression := enclosed-expression
//              or   dot-expression '.' enclosed-expression
Node Parser::parseDotExpression(Node modifiers) {
    if (auto node = parseArgumentizedExpression(modifiers)) {
        while (isaDot(cur.pos)) {
            auto dotexpr = mem.New<SyntaxDotExpression>(node->pos, modifiers, node, *cur.pos);
            node->modifiers = nullptr;
            cur.advance(); // Past '.'
            dotexpr->rhs = parseArgumentizedExpression(nullptr);
            node = dotexpr;
        }
        return node;
    }
    return nullptr;
}
//------------------------------------------------------------------------------------------------
// encolosed-expr := term | call-expression | index-expression | typename-expression
Node Parser::parseArgumentizedExpression(Node modifiers) {
    if (auto node = parseTerm(modifiers)) {
        while (!isaNewLine(cur.prev)) {
            if (isOpenParen(cur.pos)) {
                node = parseCallExpression(node);
            } else if (isOpenBracket(cur.pos)) {
                node = parseIndexExpression(node);
            } else if (isOpenAngle(cur.pos)) {
                node = parseTypeNameExpression(node);
            } else if (isOpenCurly(cur.pos)) {
                node = parseObjectExpression(node);
            } else {
                break;
            }
        }
        return node;
    }
    return nullptr;
}
// call-expression := expression '(' [arguments] ')'
Node Parser::parseCallExpression(Node name) {
    auto node = mem.New<SyntaxCallExpression>(name->pos, name->modifiers, name);
    name->modifiers = nullptr;
    auto       open = cur.pos;
    node->arguments = mem.New<SyntaxCommaList>(*open, nullptr);
    cur.advance(); // Past '('.
    if (!isCloseParen(cur.pos)) {
        parseArguments(node->arguments->nodes);
    } if (isCloseParen(cur.pos)) {
        node->arguments->close = cur.pos;
        cur.advance(); // Past ')'.
    } else {
        err(open, "unmatched %t", open);
    }
    return node;
}
// index-expression := expression '[' [arguments] ']'
Node Parser::parseIndexExpression(Node name) {
    auto node = mem.New<SyntaxIndexExpression>(name->pos, name->modifiers, name);
    name->modifiers = nullptr;
    auto       open = cur.pos;
    node->arguments = mem.New<SyntaxCommaList>(*open, nullptr);
    cur.advance(); // Past '['.
    if (!isCloseBracket(cur.pos)) {
        parseArguments(node->arguments->nodes);
    } if (isCloseBracket(cur.pos)) {
        node->arguments->close = cur.pos;
        cur.advance(); // Past ']'.
    } else {
        err(open, "unmatched %t", open);
    }
    return node;
}
// typename-expression := expression '<' [arguments] '>'
Node Parser::parseTypeNameExpression(Node name) {
    auto node = mem.New<SyntaxTypeNameExpression>(name->pos, name->modifiers, name);
    name->modifiers = nullptr;
    auto       open = cur.pos;
    node->arguments = mem.New<SyntaxCommaList>(*open, nullptr);
    cur.advance(); // Past '<'.
    if (!isCloseAngle(cur.pos)) {
        parseArguments(node->arguments->nodes);
    } if (isCloseAngle(cur.pos)) {
        node->arguments->close = cur.pos;
        cur.advance(); // Past '>'.
    } else {
        err(open, "unmatched %t", open);
    }
    return node;
}
Node Parser::parseObjectExpression(Node name) {
    auto node = mem.New<SyntaxObjectExpression>(name->pos, name->modifiers, name);
    name->modifiers = nullptr;
    auto       open = cur.pos;
    node->arguments = mem.New<SyntaxCommaList>(*open, nullptr);
    cur.advance(); // Past '{'.
    if (!isCloseCurly(cur.pos)) {
        parseArguments(node->arguments->nodes);
    } if (isCloseCurly(cur.pos)) {
        node->arguments->close = cur.pos;
        cur.advance(); // Past '}'.
    } else {
        err(open, "unmatched %t", open);
    }
    return node;
}
// arguments := argument [',' argument]+
void Parser::parseArguments(Nodes list) {
    while (true) {
        if (auto node = parseArgument()) {
            list.append(node);
        } if (isaComma(cur.pos)) {
            cur.advance(); // Past ','
        } else {
            break;
        }
    }
}
// argument := modifiers? id '=' expression
//        or   modifiers? id ':' expression ['=' expression]
//        or   expression
Node Parser::parseArgument() {
    auto modifiers = parseModifiers();
    if (isanIdentifier(cur.pos)) {
        auto   id = cur.pos;
        auto peek = cur.peek();
        if (isAssign(peek)) {
            cur.advance(); // Past {id}. Now on '='.
            auto name = mem.New<SyntaxIdentifier>(*id, nullptr, ids.get(id->value()));
            auto   nv = mem.New<SyntaxNameValue>(modifiers ? modifiers->pos : *id, modifiers, name);
            cur.advance(); // Past '='.
            nv->value = parseExpression();
            return nv;
        } if (isaColon(peek)) {
            cur.advance(); // Past {id}. Now on ':'.
            auto name = mem.New<SyntaxIdentifier>(*id, nullptr, ids.get(id->value()));
            auto nv = mem.New<SyntaxNameValue>(modifiers ? modifiers->pos : *id, modifiers, name);
            cur.advance(); // Past ':'.
            nv->typeName = parseExpression();
            if (isAssign(cur.pos)) {
                cur.advance(); // Past '='.
                nv->value = parseExpression();
            }
            return nv;
        }
    }
    return parseExpression(modifiers);
}
//------------------------------------------------------------------------------------------------
// term := literal | var-decl
//       | id
Node Parser::parseTerm(Node modifiers) {
    if (auto node = parseLiteral(modifiers)) {
        return node;
    } if (modifiers) {
        if (auto node = parseVariable(modifiers)) {
            return node;
        }
    }
    return parseIdentifier(modifiers);
}
Node Parser::parseLiteral(Node modifiers) {
    switch (cur.pos->keyword) {
        case Keyword::void_: {
            auto node = mem.New<SyntaxVoid>(*cur.pos, modifiers);
            cur.advance(); // Past 'void' keyword.
            return node;
        }
        case Keyword::Null: {
            auto node = mem.New<SyntaxNull>(*cur.pos, modifiers);
            cur.advance(); // Past 'null' keyword.
            return node;
        }
        case Keyword::False: {
            auto node = mem.New<SyntaxBoolean>(*cur.pos, modifiers, false);
            cur.advance(); // Past 'false' keyword.
            return node;
        }
        case Keyword::True: {
            auto node = mem.New<SyntaxBoolean>(*cur.pos, modifiers, true);
            cur.advance(); // Past 'true' keyword.
            return node;
        }
    } switch (cur.pos->kind) {
        case Tok::Decimal:     return parseDecimal(modifiers);
        case Tok::Hexadecimal: return parseHexadecimal(modifiers);
        case Tok::Binary:      return parseBinary(modifiers);
        case Tok::Octal:       return parseOctal(modifiers);
        case Tok::Float:       return parseFloat(modifiers);
    }
    return nullptr;
}
// variable := modifiers id '=' expression
//          or modifiers id ':' expression ['=' expression]
Node Parser::parseVariable(Node modifiers) {
    if (isanIdentifier(cur.pos)) {
        auto   id = cur.pos;
        auto peek = cur.peek();
        if (isAssign(peek)) {
            auto name = mem.New<SyntaxIdentifier>(*id, nullptr, ids.get(id->value()));
            auto node = mem.New<SyntaxNameValue>(modifiers ? modifiers->pos : *id, modifiers, name);
            cur.advance(); // Past id.
            cur.advance(); // Past '='.
            node->value = parseExpression();
            return node;
        } if (isaColon(peek)) {
            auto name = mem.New<SyntaxIdentifier>(*id, nullptr, ids.get(id->value()));
            auto node = mem.New<SyntaxNameValue>(modifiers ? modifiers->pos : *id, modifiers, name);
            cur.advance(); // Past id.
            cur.advance(); // Past ':'.
            node->typeName = parseExpression();
            if (isAssign(cur.pos)) {
                cur.advance(); // Past '='.
                node->value = parseExpression();
            }
            return node;
        }
    }
    return nullptr;
}
Node Parser::parseIdentifier(Node modifiers) {
    if (isanIdentifier(cur.pos)) {
        auto node = mem.New<SyntaxIdentifier>(*cur.pos, modifiers, ids.get(cur.pos->value()), cur.pos->keyword);
        cur.advance();
        return node;
    }
    err(cur.pos, "expected a term, not the %tn token %t", cur.pos->kind, cur.pos);
    return nullptr;
}
//------------------------------------------------------------------------------------------------
// full-name := id | id ['.' full-name] | '.' id ['.' full-name]
Node Parser::parseFullName(Node modifiers) {
    auto start = cur.pos;
    Node node{};
    if (isaDot(start)) {
        // Do nothing. Let lhs be null.
    } else {
        node = (SyntaxNode*)mem.New<SyntaxIdentifier>(*start, nullptr, ids.get(start->value()));
        cur.advance(); // Past id.
    } while (isaDot(cur.pos)) {
        auto dot = cur.pos;
        auto rhs = cur.advance();
        if (isanIdentifier(rhs)) {
            auto id = mem.New<SyntaxIdentifier>(*rhs, nullptr, ids.get(rhs->value()));
            node = mem.New<SyntaxDotExpression>(*start, nullptr, node, *dot, id);
            cur.advance(); // Past id.
        } else {
            err(rhs, "expected an identifier after %t, not %t", dot, rhs);
            break;
        }
    } if (node) {
        node->modifiers = modifiers;
    }
    return node;
}
//------------------------------------------------------------------------------------------------
Node Parser::parseModifiers() {
    Node node{};
    for (auto pos = cur.pos; isaModifier(cur.pos); pos = cur.advance()) {
        auto modifier = mem.New<SyntaxModifier>(*pos, pos->keyword);
        if (node) {
            if (node->kind == SyntaxKind::Modifiers) {
                auto modifiers = (SyntaxModifiers*)node;
                modifiers->nodes.append(modifier);
            } else {
                auto modifiers = mem.New<SyntaxModifiers>(node->pos);
                modifiers->nodes.append((SyntaxModifier*)node);
                modifiers->nodes.append(modifier);
                node = modifiers;
            }
        } else {
            node = modifier;
        }
    }
    return node;
}
Node Parser::empty(Token pos, Node modifiers) {
    return mem.New<SyntaxEmpty>(*pos, modifiers);
}
} // namespace exy