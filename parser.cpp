#include "pch.h"
#include "parser.h"

#include "src.h"

namespace exy {
using  Pos = const SourceToken*;
using Node = SyntaxNode*;
//----------------------------------------------------------
struct Is {
    auto ModifierListSyntax(auto node) { return node->kind == SyntaxKind::ModifierList ? (exy::ModifierListSyntax*)node : nullptr; }
    auto CommaSeparatedSyntax(auto node) { return node->kind == SyntaxKind::CommaSeparated ? (exy::CommaSeparatedSyntax*)node : nullptr; }
    auto PointerOrReferenceSyntax(auto node) { return node->kind == SyntaxKind::UnarySuffix ? (((UnarySuffixSyntax*)node)->op.kind == Tok::Pointer || ((UnarySuffixSyntax*)node)->op.kind == Tok::Reference) ? (UnarySuffixSyntax*)node : nullptr : nullptr; }

    auto NotEndOfFile(auto pos) { return pos->kind != Tok::EndOfFile; }
    auto NotCloseCurly(auto pos) { return pos->kind != Tok::CloseCurly; }
    auto NotCloseAngle(auto pos) { return pos->kind != Tok::CloseAngle; }
    auto NotCloseBracket(auto pos) { return pos->kind != Tok::CloseBracket; }
    auto NotCloseParen(auto pos) { return pos->kind != Tok::CloseParen; }
    auto NotCloseCurlyHash(auto pos) { return pos->kind != Tok::CloseCurlyHash; }
    auto NotOpenCurly(auto pos) { return pos->kind != Tok::OpenCurly; }

    auto Quote(auto pos) { return pos->kind >= Tok::SingleQuote && pos->kind <= Tok::RawDoubleQuote; }
    auto kwInfix(auto pos) { return pos->keyword >= Keyword::As && pos->keyword <= Keyword::NotIn; }
    auto BinaryOp(auto pos) { return pos->kind >= Tok::OrAssign && pos->kind <= Tok::Exponentiation; }
    auto TernaryOp(auto pos) { return pos->kind == Tok::Question ||  pos->keyword == Keyword::If; }
    auto InfixOp(auto pos) { return BinaryOp(pos) || TernaryOp(pos) || kwInfix(pos); }
    auto RightAssociative(auto pos) { return kwInfix(pos) || TernaryOp(pos) || pos->kind == Tok::Exponentiation; }
    auto UnaryPrefixOp(auto pos) { return pos->kind >= Tok::UnaryMinus && pos->kind <= Tok::PlusPlus; }

    auto Modifier(auto pos) { return pos->keyword > Keyword::_begin_modifiers && pos->keyword < Keyword::_end_modifiers; }
    auto UDT(auto pos) { return pos->keyword > Keyword::_begin_udts && pos->keyword < Keyword::_end_udts; }
    auto Primitive(auto pos) { return pos->keyword >= Keyword::Void && pos->keyword <= Keyword::UInt64x8; }
    auto Function(auto pos) { return pos->keyword >= Keyword::Lambda && pos->keyword <= Keyword::Blob; }
    auto Identifier(auto pos) { return pos->kind == Tok::Text && pos->keyword == Keyword::None; }

    auto CompilerKeyword(auto pos) { return pos->keyword > Keyword::_begin_compiler_keywords && pos->keyword < Keyword::_end_compiler_keywords; }

#define ZM(zName, zText) auto zName(auto pos) { return pos->kind == Tok::zName; }
    DeclarePunctuationTokens(ZM)
    DeclareGroupingTokens(ZM)
    DeclareOperatorTokens(ZM)
    DeclareTextTokens(ZM)
#undef ZM


#define ZM(zName, zText) auto kw##zName(auto pos) { return pos->keyword == Keyword::zName; }
    DeclareKeywords(ZM)
    DeclareModifiers(ZM)
    DeclareUserDefinedTypeKeywords(ZM)
    DeclareBuiltinTypeKeywords(ZM)
    DeclareCompileTimeKeywords(ZM)
#undef ZM
} is{};

//----------------------------------------------------------
Pos Parser::Cursor::advance() {
    Assert(pos <= end);
    prev = pos;
    if (pos == end) {
        return end;
    }
    if (next == nullptr) {
        next = pos;
        if (skipWhiteSpace()) {
            pos = next;
        }
    } else {
        pos = next;
    }
    if (next == end) {
        pos = next;
        return pos;
    }
    Assert(next < end);
    ++next;
    if (skipWhiteSpace()) {
        return pos;
    }
    return pos;
}

Pos Parser::Cursor::skipWhiteSpace() {
    while (true) {
        if (is.Space(next) || is.NewLine(next) || is.SingleLineComment(next) || is.MultiLineComment(next)) {
            ++next;
        } else {
            return next;
        }
    }
    Assert(!is.OpenSingleLineComment(next) && !is.OpenMultiLineComment(next));
    return nullptr;
}

bool Parser::Cursor::hasNewLineAfter(Pos cur) {
    if (cur == nullptr) {
        cur = pos;
    }
    auto p = cur + 1;
    while (p < end) {
        if (is.SingleLineComment(p)) {
            break; // Because a single comment is always followed by NL or EOF.
        }
        if (is.MultiLineComment(p)) {
            ++p;
        } else if (is.NewLine(p)) {
            return true;
        } else {
            break;
        }
    }
    return false;
}

bool Parser::Cursor::hasNewLineBefore(Pos cur) {
    if (cur == nullptr) {
        cur = pos;
    }
    auto p = cur - 1;
    while (p >= start) {
        // Don't check for single-line comment because it is always followed by NL.
        if (is.MultiLineComment(p)) {
            --p;
        } else if (is.NewLine(p)) {
            return true;
        } else {
            break;
        }
    }
    return false;
}
//----------------------------------------------------------

Parser::Parser(SyntaxFile &file)
    : file(file), cursor(file.src.tokens.start(), file.src.tokens.end()), 
    mem(compiler.syntaxTree->mem) {
    Assert(is.EndOfFile(cursor.end));
}

void Parser::dispose() {

}

void Parser::run() {
    cursor.advance(); // To set {cursor.pos}.
    while (is.NotEndOfFile(cursor.pos)) {
        auto pos = cursor.pos;
        if (auto node = parseStatement()) {
            file.nodes.append(node);
        } else if (pos == cursor.pos) { // Cursor did not move.
            cursor.advance();
        }
    }
}

Node Parser::parseStatement() {
    auto modifiers = parseModifiers();
    if (modifiers != nullptr) {
        if (is.OpenCurly(cursor.pos)) {
            return parseBlock(modifiers);
        }
        if (is.UDT(cursor.pos)) {
            return parseUDT(modifiers);
        }
        if (is.Identifier(cursor.pos)) {
            return parseVariable(modifiers);
        }
        if (is.OpenParen(cursor.pos)) {
            if (modifiers->kind == SyntaxKind::Modifier && 
                ((ModifierSyntax*)modifiers)->pos.keyword == Keyword::Synchronized) {
                return parseBlockWithArguments(modifiers);
            }
            return parseMultiVariable(modifiers);
        }
        if (is.kwFrom(cursor.pos)) {
            return parseExternsBlock(modifiers);
        }
        if (is.kwIf(cursor.pos)) {
            return parseIf(modifiers);
        }
        if (is.kwDefine(cursor.pos)) {
            return parseDefine(modifiers);
        }
        syntax_error(modifiers, "dangling modifiers: expected a block, a UDT or variable declaration after modifiers");
        return modifiers;
    }
    switch (cursor.pos->keyword) {
        case Keyword::Module:
            return parseModule();
        case Keyword::Import:
        case Keyword::Export:
            return parseImport();
        case Keyword::Define:
            return parseDefine(nullptr);
        case Keyword::From:
            return parseExternsBlock(nullptr);
        case Keyword::If:
            return parseIf(nullptr);
        case Keyword::Switch:
            return parseSwitch();
        case Keyword::Break:
        case Keyword::Continue:
            return parseBreakOrContinue();
        case Keyword::Throw:
            return parseThrow();
        case Keyword::Return:
            return parseReturn();
        case Keyword::Yield:
            return parseYield();
        case Keyword::Assert:
            return parseAssert();
        case Keyword::For:
            return parseFor();
        case Keyword::Await: if (is.kwFor(cursor.next)) {
            return parseAwaitFor();
        } break;
        case Keyword::While:
            return parseWhile();
        case Keyword::Do:
            return parseDoWhile();
        case Keyword::Defer:
            return parseDefer();
        case Keyword::Using:
            return parseUsing();
    }
    if (is.OpenCurly(cursor.pos)) {
        return parseBlock(nullptr);
    }
    return parseExpression(ctxLhsExpr);
}

Node Parser::parseModule() {
    auto node = mem.New<ModuleSyntax>(*cursor.pos);
    cursor.advance(); // Past 'module' keyword.
    node->name = parseModuleName();
    if (is.kwAs(cursor.pos)) { // Keyword 'as'.
        node->kwAs = cursor.pos;
        cursor.advance(); // Past 'as' keyword.
        if (is.Identifier(cursor.pos)) {
            node->system = mem.New<IdentifierSyntax>(*cursor.pos);
            cursor.advance(); // Past identifier.
        } else {
            syntax_error(cursor.pos, "expected identifier, not %tok", cursor.pos);
        }
    }
    if (file.moduleStatement == nullptr) {
        file.moduleStatement = node;
    } else {
        syntax_error(node, "a module cannot contain more than one module statement");
    }
    return node;
}

Node Parser::parseImport() {
    auto node = mem.New<ImportSyntax>(*cursor.pos);
    cursor.advance(); // Past 'import' | 'export' keyword.
    if (is.Multiply(cursor.pos)) {
        node->name = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past '*'.
    } else {
        node->name = parseModuleName();
    }
    if (is.kwAs(cursor.pos)) {
        parseImportAlias(node);
    } else if (is.kwFrom(cursor.pos)) {
        parseImportSource(node);
    }
    if (is.Comma(cursor.pos)) {
        return parseImportList(node);
    }
    return node;
}

void Parser::parseImportAlias(ImportSyntax *node) {
    if (node->kwAs != nullptr) {
        syntax_error(cursor.pos, "unexpected keyword %tok", cursor.pos);
        return;
    }
    node->kwAs = cursor.pos;
    cursor.advance(); // Past 'as' keyword.
    if (is.Identifier(cursor.pos)) {
        node->alias = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past identifier.
    } else {
        syntax_error(cursor.pos, "expected identifier, not %tok", cursor.pos);
    }
    if (is.kwFrom(cursor.pos)) {
        parseImportSource(node);
    }
}

void Parser::parseImportSource(ImportSyntax *node) {
    if (node->kwFrom != nullptr) {
        syntax_error(cursor.pos, "unexpected keyword %tok", cursor.pos);
        return;
    }
    node->kwFrom = cursor.pos;
    cursor.advance(); // Past 'from' keyword.
    if (is.Identifier(cursor.pos)) {
        node->source = parseModuleName();
    } else {
        syntax_error(cursor.pos, "expected identifier, not %tok", cursor.pos);
    }
    if (is.kwAs(cursor.pos)) {
        parseImportAlias(node);
    }
}

Node Parser::parseImportList(Node first) {
    auto list = mem.New<CommaSeparatedSyntax>(first);
    while (is.Comma(cursor.pos)) {
        cursor.advance(); // Past ','
        auto  node = mem.New<ImportSyntax>(first->pos);
        node->name = parseModuleName();
        if (is.kwAs(cursor.pos)) {
            parseImportAlias(node);
        }
        list->nodes.append(node);
    }
    if (is.kwFrom(cursor.pos)) {
        Cursor state = cursor;
        for (auto i = 0; i < list->nodes.length; i++) {
            cursor = state;
            auto node = (ImportSyntax*)list->nodes.items[i];
            parseImportSource(node);
        }
    }
    return list;
}

Node Parser::parseModuleName() {
    if (is.Identifier(cursor.pos)) {
        auto node = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past identifier.
        if (is.Dot(cursor.pos)) {
            return parseDotModuleName(node);
        }
        return node;
    }
    syntax_error(cursor.pos, "expected identifier, not %tok", cursor.pos);
    return nullptr;
}

Node Parser::parseDotModuleName(Node node) {
    while (is.Dot(cursor.pos)) {
        auto dot = cursor.pos;
        cursor.advance(); // Past '.'
        if (is.Identifier(cursor.pos)) {
            node = mem.New<DotSyntax>(node, *dot, mem.New<IdentifierSyntax>(*cursor.pos));
            cursor.advance(); // Past identifier.
        } else {
            syntax_error(cursor.pos, "expected identifier, not %tok", cursor.pos);
            break;
        }
    }
    return node;
}

Node Parser::parseDefine(Node modifiers) {
    auto node = mem.New<DefineSyntax>(modifiers, *cursor.pos);
    cursor.advance(); // Past 'define' keyword.
    if (is.Identifier(cursor.pos)) {
        node->name = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past identifier.
        node->value = parseExpressionList(ctxLhsExpr);
    } else {
        syntax_error(cursor.pos, "expected identifier, not %tok", cursor.pos);
    }
    return node;
}

Node Parser::parseExternsBlock(Node modifiers) {
    auto block = mem.New<ExternBlockSyntax>(*cursor.pos, modifiers);
    cursor.advance(); // Past 'from' keyword.
    block->name = parseModuleName();
    if (!is.kwImport(cursor.pos)) {
        syntax_error(cursor.pos, "expected 'import', not %tok", cursor.pos);
        return block;
    }
    cursor.advance(); // Past 'import' keyword.
    if (!is.OpenCurly(cursor.pos)) {
        syntax_error(cursor.pos, "expected '{', not %tok", cursor.pos);
        return block;
    }
    block->open = cursor.pos;
    cursor.advance(); // Past '{'.
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseCurly(cursor.pos)) {
        auto start = cursor.pos;
        if (auto node = parseStatement()) {
            block->nodes.append(node);
        } else if (start == cursor.pos) {
            cursor.advance();
        }
    }
    if (is.CloseCurly(cursor.pos)) {
        block->close = cursor.pos;
        cursor.advance(); // Past '}'.
    } else {
        syntax_error(block->open, "unmatched %tok", &block->open);
    }
    return block;
}

Node Parser::parseStructure(Node modifiers) {
    auto node = mem.New<StructureSyntax>(modifiers, *cursor.pos);
    cursor.advance(); // Past 'struct' ... 'enum' keyword.
    if (is.Identifier(cursor.pos)) {
        parseStructureName(node);
    }
    if (is.OpenBracket(cursor.pos)) {
        node->attributes = parseStructureAttributes();
    }
    if (is.Colon(cursor.pos)) {
        node->supersOp = cursor.pos;
        node->supers = parseStructureSupers();
    }
    if (is.OpenCurly(cursor.pos)) {
        node->body = parseBlock(nullptr);
    } else if (is.SemiColon(cursor.pos)) {
        node->body = mem.New<EmptySyntax>(*cursor.pos);
        cursor.advance(); // Past ';'
    } else {
        syntax_error(cursor.pos, "expected '{' or ';', not %tok", cursor.pos);
    }
    return node;
}

void Parser::parseStructureName(StructureSyntax *node) {
    node->name = mem.New<IdentifierSyntax>(*cursor.pos);
    cursor.advance(); // Past identifier.
    if (is.OpenAngle(cursor.pos)) {
        node->parameters = (AngledSyntax*)parseStructureParameters();
    }
}

Node Parser::parseStructureParameters() {
    auto list = mem.New<AngledSyntax>(*cursor.pos);
    cursor.advance(); // Past '<'
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseAngle(cursor.pos)) {
        if (is.Identifier(cursor.pos)) {
            Node node{};
            auto id = mem.New<IdentifierSyntax>(*cursor.pos);
            cursor.advance(); // Past identifier
            if (is.Assign(cursor.pos)) {
                auto nv = mem.New<NameValueSyntax>(id, *cursor.pos);
                cursor.advance(); // Past '='
                nv->value = parseExpression(ctxTypeName);
                node = nv;
            } else {
                node = id;
            }
            if (list->value == nullptr) {
                list->value = node;
            } else if (auto comma = is.CommaSeparatedSyntax(list->value)) {
                comma->nodes.append(node);
            } else {
                comma = mem.New<CommaSeparatedSyntax>(list->value);
                comma->nodes.append(node);
                list->value = comma;
            }
        } else {
            syntax_error(cursor.pos, "expected identifier, not %tok", cursor.pos);
            cursor.advance(); // Past bad token.
        }
        if (is.Comma(cursor.pos)) {
            cursor.advance(); // Past ','
        }
    }
    if (is.CloseAngle(cursor.pos)) {
        list->close = cursor.pos;
        cursor.advance(); // Past '>'.
    } else {
        syntax_error(list->pos, "unmatched %tok", &list->pos);
    }
    return list;
}

Node Parser::parseStructureAttributes() {
    auto list = mem.New<BracketedSyntax>(*cursor.pos);
    cursor.advance(); // Past '['
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseBracket(cursor.pos)) {
        if (is.Identifier(cursor.pos)) {
            Node node{};
            auto id = mem.New<IdentifierSyntax>(*cursor.pos);
            cursor.advance(); // Past identifier
            if (is.Assign(cursor.pos)) {
                auto nv = mem.New<NameValueSyntax>(id, *cursor.pos);
                cursor.advance(); // Past '='
                nv->value = parseExpression(ctxRhsExpr);
                node = nv;
            } else {
                node = id;
            }
            if (list->value == nullptr) {
                list->value = node;
            } else if (auto comma = is.CommaSeparatedSyntax(list->value)) {
                comma->nodes.append(node);
            } else {
                comma = mem.New<CommaSeparatedSyntax>(list->value);
                comma->nodes.append(node);
                list->value = comma;
            }
        } else {
            syntax_error(cursor.pos, "expected identifier, not %tok", cursor.pos);
            cursor.advance(); // Past bad token.
        }
        if (is.Comma(cursor.pos)) {
            cursor.advance(); // Past ','
        }
    }
    if (is.CloseBracket(cursor.pos)) {
        list->close = cursor.pos;
        cursor.advance(); // Past ']'.
    } else {
        syntax_error(list->pos, "unmatched %tok", &list->pos);
    }
    return list;
}

Node Parser::parseStructureSupers() {
    cursor.advance(); // Past ':' or '->'.
    if (auto node = parseExpression(ctxTypeName)) {
        while (is.Comma(cursor.pos)) {
            cursor.advance(); // Past ','.
            if (auto item = parseExpression(ctxTypeName)) {
                if (auto list = is.CommaSeparatedSyntax(node)) {
                    list->nodes.append(item);
                } else {
                    list = mem.New<CommaSeparatedSyntax>(node);
                    list->nodes.append(item);
                    node = list;
                }
            }
        }
        return node;
    }
    return nullptr;
}

Node Parser::parseFunction(Node modifiers) {
    auto node = mem.New<FunctionSyntax>(modifiers, *cursor.pos);
    currentFunction = { &currentFunction, node };
    cursor.advance(); // Past 'fn' ... 'blob' keyword.
    parseFunctionName(node);
    if (is.OpenParen(cursor.pos)) {
        node->parameters = (ParenthesizedSyntax*)parseFunctionParameters();
    }
    if (is.Colon(cursor.pos) || is.DashArrow(cursor.pos)) {
        node->fnreturnOp = cursor.pos;
        node->fnreturn = parseStructureSupers();
    } else if (is.kwExtern(&node->pos)) {
        syntax_error(cursor.pos, "expected ':' or '->' after %kw declaration, not %tok", 
                     node->pos.keyword, cursor.pos);
    }
    if (is.kwExtern(&node->pos)) {
        if (is.SemiColon(cursor.pos)) {
            node->body = mem.New<EmptySyntax>(*cursor.pos);
            cursor.advance(); // Past ';'
        } else if (is.OpenCurly(cursor.pos) || is.Assign(cursor.pos) || is.AssignArrow(cursor.pos) || 
                   is.HashOpenCurly(cursor.pos)) {
            syntax_error(cursor.pos, "expected ';', not %tok", cursor.pos);
        } else {
            node->body = mem.New<EmptySyntax>(*cursor.pos);
        }
    } else if (is.OpenCurly(cursor.pos)) {
        node->body = parseBlock(nullptr);
    } else if (is.SemiColon(cursor.pos)) {
        node->body = mem.New<EmptySyntax>(*cursor.pos);
        cursor.advance(); // Past ';'
    } else if (is.Assign(cursor.pos) || is.AssignArrow(cursor.pos)) {
        node->bodyOp = cursor.pos;
        cursor.advance(); // Past '=' | '=>'.
        node->body = parseExpression(ctxLhsExpr);
    } else if (is.HashOpenCurly(cursor.pos)) {
        node->body = parseTextBlock();
    } else {
        syntax_error(cursor.pos, "expected '{', ';', '=' or '=>', not %tok", cursor.pos);
    }
    if (currentFunction.prev) {
        currentFunction = *currentFunction.prev;
    }
    return node;
}

void Parser::parseFunctionName(FunctionSyntax *node) {
    if (is.kwUrlHandler(&node->pos)) {
        parseUrlHandlerName(node);
    } else if (is.Identifier(cursor.pos)) {
        node->name = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past identifier.
    } else if (is.Quote(cursor.pos)) {
        node->name = parseQuoted();
    } else if (is.OpenParen(cursor.pos) && is.CloseParen(cursor.next)) {
        auto    open = cursor.pos;
        auto   close = cursor.next;
        Cursor state = cursor;
        cursor.advance(); // Past '('
        cursor.advance(); // Past ')'
        if (is.OpenParen(cursor.pos)) {
            ((SourceToken*)open)->kind  = Tok::OpenCloseParen;
            ((SourceToken*)close)->kind = Tok::OpenCloseParen;
            node->opName = open;
        } else {
            cursor = state;
        }
    } else if (is.OpenBracket(cursor.pos) && is.CloseBracket(cursor.next)) {
        auto    open = cursor.pos;
        auto   close = cursor.next;
        Cursor state = cursor;
        cursor.advance(); // Past '['
        cursor.advance(); // Past ']'
        if (is.OpenParen(cursor.pos)) {
            ((SourceToken*)open)->kind  = Tok::OpenCloseBracket;
            ((SourceToken*)close)->kind = Tok::OpenCloseBracket;
            node->opName = open;
        } else {
            cursor = state;
        }
    } else {
        parseFunctionOperatorName(node);
    }
}

void Parser::parseUrlHandlerName(FunctionSyntax *node) {
    if (is.Quote(cursor.pos)) {
        node->name = parseQuoted();
        return;
    }
    auto start = cursor.pos;
    if (is.Identifier(start)) {
        static const Identifier protocols[] = {
            ids.kw_http, ids.kw_https, ids.kw_ws, ids.kw_wss
        };
        static const Identifier verbs[] = {
            ids.kw_GET, ids.kw_POST, ids.kw_DELETE, ids.kw_PUT, 
            ids.kw_HEAD, ids.kw_CONNECT, ids.kw_OPTIONS, ids.kw_TRACE, ids.kw_PATCH
        };
        auto protocol = start->sourceValue();
        for (auto i = 0; i < _countof(protocols); i++) {
            if (protocols[i]->cmp(protocol) == 0) {
                node->webProtocol = mem.New<IdentifierSyntax>(*start);
                cursor.advance(); // Past web-protocol
                start = cursor.pos;
                break;
            }
        }
        if (is.Identifier(start)) {
            auto verb = start->sourceValue();
            for (auto i = 0; i < _countof(verbs); i++) {
                if (verbs[i]->cmp(verb) == 0) {
                    node->httpVerb = mem.New<IdentifierSyntax>(*start);
                    cursor.advance(); // Past http-verb.
                    break;
                }
            }
        }
        if (is.Quote(cursor.pos)) {
            node->name = parseQuoted();
            return;
        }
    }
    start = cursor.pos;
    while (true) {
        if (is.Text(cursor.pos) || is.Divide(cursor.pos)) {
            cursor.advance();
        } else {
            break;
        }
    }
    auto end = cursor.pos;
    String value{ start->pos.range.start.text, end->pos.range.start.text };
    value.removeTrailingSpaces();
    if (value.isNotEmpty()) {
        node->name = mem.New<TextSyntax>(*start, value, *end);
    } else {
        syntax_error(cursor.pos, "expected a urlhandler name before %tok", cursor.pos);
    }
}

void Parser::parseFunctionOperatorName(FunctionSyntax *node) {
    if (is.BinaryOp(cursor.pos) || is.UnaryPrefixOp(cursor.pos)) {
        node->opName = cursor.pos;
        cursor.advance(); // Past operator.
    } else if (is.kwInfix(cursor.pos)) {
        node->kwName = cursor.pos;
        cursor.advance(); // Past keyword.
    }
}

Node Parser::parseFunctionParameters() {
    auto params = mem.New<ParenthesizedSyntax>(*cursor.pos);
    cursor.advance(); // Past '('.
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseParen(cursor.pos)) {
        auto modifiers = parseModifiers();
        auto       pos = cursor.pos;
        if (auto param = parseFunctionParameter(modifiers)) {
            if (params->value == nullptr) {
                params->value = param;
            } else if (auto list = is.CommaSeparatedSyntax(params->value)) {
                list->nodes.append(param);
            } else {
                list = mem.New<CommaSeparatedSyntax>(params->value);
                list->nodes.append(param);
                params->value = list;
            }
        } else if (modifiers) {
            syntax_error(modifiers, "dangling modifiers");
            ndispose(modifiers);
        } else if (pos == cursor.pos) {
            cursor.advance(); // Past bad token.
        }
        if (is.Comma(cursor.pos)) {
            cursor.advance(); // Past ','
        }
    }
    if (is.CloseParen(cursor.pos)) {
        params->close = cursor.pos;
        cursor.advance(); // Past ')'.
    } else {
        syntax_error(params->pos, "unmatched %tok", &params->pos);
    }
    return params;
}

Node Parser::parseFunctionParameter(Node modifiers) {
    if (is.Identifier(cursor.pos) || is.kwThis(cursor.pos)) {
        auto id = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past identifier.
        if (is.Ellipsis(cursor.pos)) {
            auto node = mem.New<RestParameterSyntax>(modifiers, id, *cursor.pos);
            cursor.advance(); // Past '...'
            return node;
        }
        auto node = mem.New<VariableSyntax>(modifiers, id->pos);
        node->name = id;
        if (is.Colon(cursor.pos)) {
            auto nv = mem.New<NameValueSyntax>((IdentifierSyntax*)node->name, *cursor.pos);
            cursor.advance(); // Past ':'
            nv->value = parseExpression(ctxTypeName);
            node->name = nv;
        }
        if (is.ColonAssign(cursor.pos) || is.Assign(cursor.pos)) {
            node->assign = cursor.pos;
            cursor.advance(); // Past '=' | ':='.
            node->rhs = parseExpression(ctxLhsExpr);
        }
        return node;
    } if (is.Ellipsis(cursor.pos)) {
        auto node = mem.New<RestParameterSyntax>(modifiers, *cursor.pos);
        cursor.advance(); // Past '...'
        return node;
    }
    syntax_error(cursor.pos, "expected identifier or '...', not %tok", cursor.pos);
    return nullptr;
}

Node Parser::parseModifiers() {
    Node node{};
    while (is.Modifier(cursor.pos)) {
        if (node == nullptr) {
            node = mem.New<ModifierSyntax>(*cursor.pos);
        } else if (auto list = is.ModifierListSyntax(node)) {
            list->nodes.append(mem.New<ModifierSyntax>(*cursor.pos));
        } else {
            list = mem.New<ModifierListSyntax>((ModifierSyntax*)node);
            list->nodes.append(mem.New<ModifierSyntax>(*cursor.pos));
            node = list;
        }
        cursor.advance(); // Past modifier.
    }
    return node;
}

Node Parser::parseVariable(Node modifiers, Ctx ctx) {
    auto  node = mem.New<VariableSyntax>(modifiers, *cursor.pos);
    node->name = mem.New<IdentifierSyntax>(*cursor.pos);
    cursor.advance(); // Past identifier.
    if (is.ColonAssign(cursor.pos)) {
        node->assign = cursor.pos;
        cursor.advance(); // Past ':='
        node->rhs = parseExpression(((ctx & ctxRhsExpr) != 0) ? ctx : ctxLhsExpr);
        return node;
    } 
    if (is.Colon(cursor.pos)) {
        auto nv = mem.New<NameValueSyntax>((IdentifierSyntax*)node->name, *cursor.pos);
        cursor.advance(); // Past ':'
        nv->value = parseExpression(ctxTypeName);
        node->name = nv;
    }
    if (is.Assign(cursor.pos)) {
        node->assign = cursor.pos;
        cursor.advance(); // Past '='
        node->rhs = parseExpression(((ctx & ctxRhsExpr) != 0) ? ctx : ctxLhsExpr);
    }
    return node;
}

Node Parser::parseMultiVariable(Node modifiers) {
    auto   node = mem.New<VariableSyntax>(modifiers, *cursor.pos);
    auto parens = mem.New<ParenthesizedSyntax>(*cursor.pos);
    cursor.advance(); // Past '('
    while (!is.EndOfFile(cursor.pos) && !is.CloseParen(cursor.pos)) {
        if (is.Identifier(cursor.pos)) {
            Node item{};
            auto id = mem.New<IdentifierSyntax>(*cursor.pos);
            cursor.advance(); // Past identifier.
            if (is.Colon(cursor.pos)) {
                auto nv = mem.New<NameValueSyntax>(id, *cursor.pos);
                cursor.advance(); // Past ':'
                nv->value = parseExpression(ctxTypeName);
                item = nv;
            } else {
                item = id;
            }
            if (parens->value == nullptr) {
                parens->value = item;
            } else if (auto list = is.CommaSeparatedSyntax(parens->value)) {
                list->nodes.append(item);
            } else {
                list = mem.New<CommaSeparatedSyntax>(parens->value);
                list->nodes.append(item);
                parens->value = list;
            }
            if (is.Comma(cursor.pos)) {
                cursor.advance();
            }
        } else {
            syntax_error(cursor.pos, "expected identifier, not %tok", cursor.pos);
            cursor.advance(); // Past bad token.
        }
    }
    node->name = parens;
    if (!is.CloseParen(cursor.pos)) {
        syntax_error(parens->pos, "unmatched %tok", &parens->pos);
        return node;
    }
    parens->close = cursor.pos;
    cursor.advance(); // Past ')'
    if (is.Assign(cursor.pos) || is.ColonAssign(cursor.pos)) {
        node->assign = cursor.pos;
        cursor.advance(); // Past '='
        node->rhs = parseExpression(ctxLhsExpr);
    }
    return node;
}

Node Parser::parseBlock(Node modifiers) {
    auto block = mem.New<BlockSyntax>(modifiers, *cursor.pos);
    cursor.advance(); // Past '{'.
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseCurly(cursor.pos)) {
        auto pos = cursor.pos;
        if (auto node = parseStatement()) {
            block->nodes.append(node);
        } else if (pos == cursor.pos) { // Cursor did not move.
            cursor.advance();
        }
    }
    if (is.CloseCurly(cursor.pos)) {
        block->close = cursor.pos;
        cursor.advance(); // Past '}'.
    } else {
        syntax_error(block->pos, "unmatched %tok", &block->pos);
    }
    return block;
}

Node Parser::parseBlockWithArguments(Node modifiers) {
    if (auto arguments = parseParenthesized(ctxLhsExpr)) {
        if (is.OpenCurly(cursor.pos)) {
            auto block = (BlockSyntax*)parseBlock(modifiers);
            block->arguments = (ParenthesizedSyntax*)arguments;
            return block;
        }
        syntax_error(modifiers, "dangling modifiers");
        ndispose(modifiers);
        return arguments;
    }
    syntax_error(modifiers, "dangling modifiers");
    return modifiers;
}

Node Parser::parseUDT(Node modifiers) {
    switch (cursor.pos->keyword) {
        case Keyword::Struct:
        case Keyword::Union:
        case Keyword::Enum:
            return parseStructure(modifiers);
    }
    if (is.Function(cursor.pos)) {
        return parseFunction(modifiers);
    }
    syntax_error(cursor.pos, "not a UDT, %tok", cursor.pos);
    return modifiers;
}

Node Parser::parseTextBlock() {
    auto  block = mem.New<BlockSyntax>(*cursor.pos);
    auto &nodes = block->nodes;
    cursor.advance(); // Past '#{'.
    auto start = cursor.pos;
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseCurlyHash(cursor.pos)) {
        if (is.HashOpenParen(cursor.pos) || is.HashOpenBracket(cursor.pos)) {
            auto  end = cursor.pos;
            auto value = String{ start->pos.range.start.text, end->pos.range.start.text };
            if (value.isNotEmpty()) {
                nodes.append(mem.New<TextSyntax>(*start, value, *end));
            }
            nodes.append(parseCodeBlock());
            start = cursor.pos;
        } else {
            cursor.advance();
        }
    }
    if (is.CloseCurlyHash(cursor.pos)) {
        auto   end = cursor.pos;
        auto value = String{ start->pos.range.start.text, end->pos.range.start.text };
        if (value.isNotEmpty()) {
            nodes.append(mem.New<TextSyntax>(*start, value, *end));
        }
        cursor.advance(); // Past '}#'.
    } else {
        syntax_error(block->pos, "unmatched %tok", &block->pos);
    }
    return block;
}

Node Parser::parseCodeBlock() {
    auto block = mem.New<CodeBlockSyntax>(*cursor.pos);
    auto close = is.HashOpenParen(cursor.pos) ? Tok::CloseParen : Tok::CloseBracket;
    cursor.advance(); // Past '#(' or '#['.
    while (is.NotEndOfFile(cursor.pos) && cursor.pos->kind != close) {
        auto pos = cursor.pos;
        if (auto node = parseStatement()) {
            if (block->node == nullptr) {
                block->node = node;
            } else if (auto list = is.CommaSeparatedSyntax(block->node)) {
                list->nodes.append(node);
            } else {
                list = mem.New<CommaSeparatedSyntax>(block->node);
                list->nodes.append(node);
                block->node = list;
            }
        } else if (pos == cursor.pos) {
            cursor.advance(); // Past bad token.
        }
    }
    if (cursor.pos->kind == close) {
        block->close = cursor.pos;
        cursor.advance(); // Past ')' or ']'.
    } else {
        syntax_error(block->pos, "unmatched %tok", &block->pos);
    }
    return block;
}

Node Parser::parseIf(Node modifiers) {
    auto node = mem.New<IfSyntax>(modifiers, *cursor.pos);
    cursor.advance(); // Past 'if' keyword.
    if (is.OpenCurly(cursor.pos)) {
        node->condition = mem.New<BooleanSyntax>(*cursor.pos, true);
    } else {
        node->condition = parseExpression(ctxRhsExpr);
    }
    node->iftrue = parseStatement();
    if (is.kwElse(cursor.pos)) {
        node->kwElse = cursor.pos;
        cursor.advance(); // Past 'else' keyword.
        node->ifalse = parseStatement();
    }
    return node;
}

Node Parser::parseSwitch() {
    auto node = mem.New<SwitchSyntax>(*cursor.pos);
    cursor.advance(); // Past 'switch' keyword.
    if (is.OpenCurly(cursor.pos)) {
        node->condition = mem.New<BooleanSyntax>(*cursor.pos, true);
    } else {
        node->condition = parseExpression(ctxRhsExpr);
    }
    if (is.OpenCurly(cursor.pos) || is.kwCase(cursor.pos)) {
        node->body = parseCaseList(node);
    } else {
        syntax_error(cursor.pos, "expected '{' or 'case', not %tok", cursor.pos);
    }
    return node;
}

Node Parser::parseCaseList(SwitchSyntax *parent) {
    auto block = mem.New<BlockSyntax>(*cursor.pos);
    Pos open = nullptr;
    if (is.OpenCurly(cursor.pos)) {
        open = cursor.pos;
        cursor.advance();
    }
    while (is.kwCase(cursor.pos) || is.kwDefault(cursor.pos)) {
        auto isCase = is.kwCase(cursor.pos);
        auto   node = mem.New<CaseSyntax>(*cursor.pos);
        cursor.advance(); // Past 'case'.
        if (isCase) {
            node->condition = parseCaseCondition(parent);
        } else {
            node->body = parseStatement();
        }
        block->nodes.append(node);
        if (!is.kwCase(cursor.pos) && !is.kwDefault(cursor.pos)) {
            node->body = parseStatement();
        }
    }
    if (open != nullptr) {
        if (is.CloseCurly(cursor.pos)) {
            block->close = cursor.pos;
            cursor.advance(); // Past '}'
        } else {
            syntax_error(block->pos, "unmatched %tok", &block->pos);
        }
    }
    return block;
}

Node Parser::parseCaseCondition(SwitchSyntax *parent) {
    if (is.Identifier(cursor.pos) && is.Colon(cursor.next)) {
        auto name = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past identifier.
        auto node = mem.New<NameValueSyntax>(name, *cursor.pos);
        cursor.advance(); // Past ','
        node->value = parseExpression(ctxRhsExpr);
        parent->isaTypeSwitch = true;
        if (is.Comma(cursor.pos)) {
            return parseCaseConditionList(parent, node);
        }
        return node;
    }
    if (auto node = parseExpression(ctxRhsExpr)) {
        if (is.Comma(cursor.pos)) {
            return parseCaseConditionList(parent, node);
        }
        return node;
    }
    return nullptr;
}

Node Parser::parseCaseConditionList(SwitchSyntax *parent, Node first) {
    auto list = mem.New<CommaSeparatedSyntax>(first);
    while (is.Comma(cursor.pos)) {
        cursor.advance(); // Past ','
        if (is.Identifier(cursor.pos) && is.Colon(cursor.next)) {
            auto name = mem.New<IdentifierSyntax>(*cursor.pos);
            cursor.advance(); // Past identifier.
            auto node = mem.New<NameValueSyntax>(name, *cursor.pos);
            cursor.advance(); // Past ','
            node->value = parseExpression(ctxRhsExpr); // Stop at '{'.
            list->nodes.append(node);
            parent->isaTypeSwitch = true;
        } else if (auto node = parseExpression(ctxRhsExpr)) { // Stop at '{'.
            list->nodes.append(node);
        }
    }
    return list;
}

Node Parser::parseAssert() {
    auto node = mem.New<FlowControlSyntax>(*cursor.pos);
    cursor.advance(); // Past 'assert' keyword.
    if (is.kwWith(cursor.pos)) {
        node->expression = mem.New<BooleanSyntax>(*cursor.pos, false);
        cursor.advance(); // Past 'with' keyword.
        node->with = parseWith();
    } else {
        node->expression = parseExpressionList(Ctx(ctxLhsExpr | ctxNoWith)); // Let '{' start an initializer.
    }
    if (is.kwWith(cursor.pos)) {
        node->kwWith = cursor.pos;
        cursor.advance(); // Past 'with' keyword.
        node->with = parseWith();
    }
    return node;
}

Node Parser::parseThrow() {
    auto node = mem.New<FlowControlSyntax>(*cursor.pos);
    cursor.advance(); // Past 'throw' keyword.
    if (cursor.hasNewLineAfter(&node->pos)) {
        // Do nothing.
    } else if (is.kwIf(cursor.pos)) {
        node->kwIf = cursor.pos;
        cursor.advance(); // Past 'if' keyword.
        node->expression = parseExpressionList(Ctx(ctxRhsExpr | ctxNoWith)); // Stop at '{' or 'with'.
    } else if (cursor.hasNewLineAfter()) {
        // Do nothing.
    } else {
        node->expression = parseExpressionList(Ctx(ctxLhsExpr | ctxNoWith)); // Let '{' start an initializer.
    }
    if (is.kwWith(cursor.pos)) {
        node->kwWith = cursor.pos;
        cursor.advance(); // Past 'with' keyword.
        node->with = parseStatement();
    }
    return node;
}

Node Parser::parseReturn() {
    auto node = mem.New<FlowControlSyntax>(*cursor.pos);
    cursor.advance(); // Past 'return' keyword.
    if (cursor.hasNewLineAfter(&node->pos)) {
        // Do nothing.
    } else if (is.kwIf(cursor.pos)) {
        node->kwIf = cursor.pos;
        cursor.advance(); // Past 'if' keyword.
        node->expression = parseExpression(ctxRhsExpr); // Stop at '{'.
    } else {
        node->expression = parseExpressionList(ctxLhsExpr); // Allow '{' to be an initializer.
    }
    if (auto fn = currentFunction.node) {
        ++fn->returns;
    }
    return node;
}

Node Parser::parseYield() {
    auto node = mem.New<UnaryPrefixSyntax>(*cursor.pos);
    cursor.advance(); // Past 'yield' keyword.
    if (is.kwFrom(cursor.pos)) {
        node->kwFrom = cursor.next;
        cursor.advance(); // Past 'from' keyword.
    }
    node->expression = parseExpression(ctxRhsExpr);
    if (auto fn = currentFunction.node) {
        ++fn->yields;
    } else {
        syntax_error(node, "%kw outside of a function", node->pos.keyword);
    }
    return node;
}

Node Parser::parseBreakOrContinue() {
    auto node = mem.New<FlowControlSyntax>(*cursor.pos);
    cursor.advance(); // Past 'break' | 'continue' keyword.
    if (is.kwIf(cursor.pos) && !cursor.hasNewLineBefore()) {
        node->kwIf = cursor.pos;
        cursor.advance(); // Past 'if' keyword.
        node->expression = parseExpression(ctxRhsExpr); // Stop at '{'.
    }
    return node;
}

Node Parser::parseFor() {
    auto kwFor = cursor.pos;
    cursor.advance(); // Past 'for' keyword.
    if (is.OpenCurly(cursor.pos)) {
        // 'for' '{' ...
        auto  node = mem.New<ForSyntax>(*kwFor);
        node->body = parseBlock(/* modifiers = */ nullptr);
        return parseForElse(node);
    }
    if (is.SemiColon(cursor.pos)) {
        auto node = mem.New<ForSyntax>(*kwFor);
        // 'for' ';' ...
        cursor.advance(); // Past 1st ';'.
        if (is.SemiColon(cursor.pos)) {
            // 'for' ';' ';' ...
            cursor.advance(); // Past 2nd ';'.
            if (is.OpenCurly(cursor.pos)) {
                // 'for' ';' ';' '{' ...
                node->body = parseBlock(/* modifiers = */ nullptr);
                return parseForElse(node);
            }
            // 'for' ';' ';' increment ...
            node->increment = parseExpressionList(ctxRhsExpr); // Stop at '{'.
            node->body = parseStatement();
            return parseForElse(node);
        }
        // 'for' ';' condition ...
        node->condition = parseExpressionList(ctxRhsExpr); // Stop at '{'.
        if (is.SemiColon(cursor.pos)) {
            // 'for' ';' condition ';' ...
            cursor.advance(); // Past 2nd ';'.
            if (is.OpenCurly(cursor.pos)) {
                // 'for' ';' condition ';' '{' ...
                node->body = parseStatement();
                return parseForElse(node);
            }
            // 'for' ';' condition ';' increment ...
            node->increment = parseExpressionList(ctxRhsExpr); // Stop at '{'.
            node->body = parseStatement();
            return parseForElse(node);
        }
        syntax_error(cursor.pos, "expected ';', not %tok", cursor.pos);
        return parseForElse(node);
    }
    if (is.OpenParen(cursor.pos)) { // 'for' '(' ...
        auto open = cursor.pos;
        cursor.advance(); // Past '('.
        if (is.CloseParen(cursor.pos)) {
            // 'for' '(' ')' ...
            auto node = mem.New<ForSyntax>(*kwFor);
            cursor.advance(); // Past ')'.
            node->body = parseStatement();
            return parseForElse(node);
        }
        if (is.SemiColon(cursor.pos)) {
            // 'for' '(' ';' ...
            auto node = mem.New<ForSyntax>(*kwFor);
            cursor.advance(); // Past 1st ';'.
            if (is.SemiColon(cursor.pos)) {
                // 'for' '(' ';' ';' ...
                cursor.advance(); // Past 2nd ';'.
                if (is.NotCloseParen(cursor.pos)) {
                    // 'for' '(' ';' ';' increment ...
                    node->increment = parseExpressionList(ctxLhsExpr); // Allow '{' becase we are in '('.
                }
                if (is.CloseParen(cursor.pos)) {
                    // 'for' '(' ';' ';' increment ')' ...
                    cursor.advance(); // Past ')'.
                    node->body = parseStatement();
                } else {
                    syntax_error(open, "unmatched %tok", open);
                }
            } else {
                // 'for' '(' ';' condition ...
                node->condition = parseExpressionList(ctxLhsExpr); // Allow '{'.
                if (is.SemiColon(cursor.pos)) {
                    // 'for' '(' ';' condition ';' ...
                    cursor.advance(); // Past 2nd ';'.
                    if (is.NotCloseParen(cursor.pos)) {
                        // 'for' '(' ';' condition ';' increment ...
                        node->increment = parseExpressionList(ctxLhsExpr);
                    }
                    if (is.CloseParen(cursor.pos)) {
                        cursor.advance(); // Past ')'.
                        node->body = parseStatement();
                    } else {
                        syntax_error(open, "unmatched %tok", open);
                    }
                } else {
                    syntax_error(cursor.pos, "expected ';', not %tok", cursor.pos);
                }
            }
            return parseForElse(node);
        }
        if (auto expr = parseExpressionList(Ctx(ctxLhsExpr | ctxNoIn))) { // Allow '{' but stop at 'in'.
            if (is.kwIn(cursor.pos)) {
                // 'for' '(' variables 'in' ...
                auto node = mem.New<ForInSyntax>(*kwFor, /* variables = */expr);
                node->kwIn = cursor.pos;
                cursor.advance(); // Past 'in' keyword.
                node->expression = parseExpression(ctxLhsExpr); // Take '{' because we are in '('.
                if (is.CloseParen(cursor.pos)) {
                    // 'for' '(' variables 'in' expr ')' ...
                    cursor.advance(); // Past ')'.
                    node->body = parseStatement();
                } else {
                    syntax_error(open, "unmatched %tok", open);
                }
                return parseForElse(node);
            }
            if (is.SemiColon(cursor.pos)) {
                // 'for' '(' initializer ';' ...
                auto node = mem.New<ForSyntax>(*kwFor, /* initializer = */expr);
                cursor.advance(); // Past 1st ';'.
                if (is.SemiColon(cursor.pos)) {
                    // 'for' '(' initializer ';' ';' ...
                    cursor.advance(); // Past 2nd ';'
                    if (is.CloseParen(cursor.pos)) {
                        // 'for' '(' initializer ';' ';' ')' ...
                        cursor.advance(); // Past ')'.
                        node->body = parseStatement();
                        return parseForElse(node);
                    }
                    // 'for' '(' initializer ';' ';' increment ...
                    node->increment = parseExpressionList(ctxLhsExpr);
                    if (is.CloseParen(cursor.pos)) {
                        // 'for' '(' initializer ';' ';' increment ')' ...
                        cursor.advance(); // Past ')'.
                        node->body = parseStatement();
                    } else {
                        syntax_error(open, "unmatched %tok", open);
                    }
                    return parseForElse(node);
                }
                // 'for' '(' initializer ';' condition ...
                node->condition = parseExpressionList(ctxLhsExpr);
                if (is.SemiColon(cursor.pos)) {
                    // 'for' '(' initializer ';' condition ';' ...
                    cursor.advance(); // Past 2nd ';'
                    if (is.OpenCurly(cursor.pos)) {
                        // 'for' initializer ';' condition ';' '{' ...
                        node->body = parseStatement();
                        return parseForElse(node);
                    }
                    // 'for' initializer ';' ';' increment ...
                    node->increment = parseExpressionList(ctxRhsExpr); // Stop at '{'
                    node->body = parseStatement();
                    return parseForElse(node);
                }
                syntax_error(cursor.pos, "expected ';', not %tok", cursor.pos);
                return parseForElse(node);
            }
            syntax_error(cursor.pos, "expected 'in' or ';', not %tok", cursor.pos);
            return expr;
        }
        syntax_error(cursor.pos, "expected ';', ')' or expression, not %tok", cursor.pos);
        return nullptr;
    }
    if (auto expr = parseExpressionList(Ctx(ctxRhsExpr | ctxNoIn))) {
        if (is.kwIn(cursor.pos)) {
            // 'for' variables 'in' ...
            auto node = mem.New<ForInSyntax>(*kwFor, /* variables = */ expr);
            cursor.advance(); // Past 'in' keyword.
            node->expression = parseExpressionList(ctxRhsExpr); // Stop at '{'.
            node->body = parseStatement();
            return parseForElse(node);
        }
        if (is.SemiColon(cursor.pos)) {
            // 'for' initializer ';' ...
            auto node = mem.New<ForSyntax>(*kwFor, /* initializer = */ expr);
            cursor.advance(); // Past 1st ';'.
            if (is.SemiColon(cursor.pos)) {
                // 'for' initializer ';' ';' ...
                cursor.advance(); // Past 2nd ';'.
                if (is.OpenCurly(cursor.pos)) {
                    // 'for' initializer ';' ';' '{' ...
                    node->body = parseStatement();
                } else {
                    // 'for' initializer ';' ';' increment ...
                    node->increment = parseExpressionList(ctxRhsExpr);
                    node->body = parseStatement();
                }
            } else {
                // 'for' initializer ';' condition ...
                node->condition = parseExpressionList(ctxRhsExpr);
                if (is.SemiColon(cursor.pos)) {
                    // 'for' initializer ';' condition ';' ...
                    cursor.advance(); // Past 2nd ';'.
                    if (is.OpenCurly(cursor.pos)) {
                        // 'for' initializer ';' condition ';' '{' ...
                        node->body = parseStatement();
                    } else if (is.SemiColon(cursor.pos)) {
                        // 'for' initializer ';' condition ';' ';' ...
                        cursor.advance(); // Past 3rd ';'
                        node->body = parseStatement();
                    } else {
                        // 'for' initializer ';' condition ';' increment body
                        node->increment = parseExpressionList(ctxRhsExpr);
                        node->body = parseStatement();
                    }
                    return parseForElse(node);
                } else if (is.OpenCurly(cursor.pos)) {
                    // 'for' initializer ';' condition '{' ...
                    node->body = parseStatement();
                } else {
                    syntax_error(cursor.pos, "expected ';' or '{' after condition, not %tok", cursor.pos);
                }
            }
            return parseForElse(node);
        }
        // 'for' condition statement
        auto node = mem.New<ForSyntax>(*kwFor);
        node->condition = expr;
        node->body = parseStatement();
        return parseForElse(node);
    }
    return nullptr;
}

Node Parser::parseAwaitFor() {
    auto kwAwait = cursor.pos;
    cursor.advance(); // Past 'await' keyword.
    if (auto node = parseFor()) {
        if (node->kind == SyntaxKind::ForIn) {
            auto forin = (ForInSyntax*)node;
            forin->kwAwait = kwAwait;
            if (auto fn = currentFunction.node) {
                ++fn->awaits;
            }
            return forin;
        }
        syntax_error(node, "expected 'forin' syntax");
        return node;
    }
    return nullptr;
}

Node Parser::parseForElse(ForInSyntax *node) {
    if (is.kwElse(cursor.pos)) {
        node->kwElse = cursor.pos;
        cursor.advance(); // Past 'else' keyword.
        node->ifnobreak = parseStatement();
    }
    return node;
}

Node Parser::parseForElse(ForSyntax *node) {
    if (is.kwElse(cursor.pos)) {
        node->kwElse = cursor.pos;
        cursor.advance(); // Past 'else' keyword.
        node->ifnobreak = parseStatement();
    }
    return node;
}

Node Parser::parseWhile() {
    auto node = mem.New<WhileSyntax>(*cursor.pos);
    cursor.advance(); // Past 'while' keyword.
    if (is.OpenCurly(cursor.pos)) { // while true ...
        node->condition = mem.New<BooleanSyntax>(*cursor.pos, true);
    } else { // 'while' condition ...
        node->condition = parseExpressionList(ctxRhsExpr); // Stop at '{'.
    }
    node->body = parseStatement();
    if (is.kwElse(cursor.pos)) {
        // 'while' [condition] body 'else' ...
        node->kwElse = cursor.pos;
        cursor.advance(); // Past 'else' keyword.
        node->ifnobreak = parseStatement();
    }
    return node;
}

Node Parser::parseDoWhile() {
    auto node = mem.New<DoWhileSyntax>(*cursor.pos);
    cursor.advance(); // Past 'do' keyword.
    if (!is.kwWhile(cursor.pos)) {
        // 'do' body ...
        node->body = parseStatement();
    }
    if (is.kwWhile(cursor.pos)) {
        // 'do' [body] 'while' ...
        node->kwWhile = cursor.pos;
        cursor.advance(); // Past 'while' keyword.
        node->condition = parseExpressionList(ctxRhsExpr); // Stop at '{'.
    } else {
        syntax_error(cursor.pos, "expected 'while'  keyword, not %tok", cursor.pos);
    }
    if (is.kwElse(cursor.pos)) {
        // 'do' [body] 'while' condition 'else' ...
        node->kwElse = cursor.pos;
        cursor.advance(); // Past 'else' keyword.
        node->ifalse = parseStatement();
    }
    return node;
}

Node Parser::parseDefer() {
    auto node = mem.New<DeferSyntax>(*cursor.pos);
    cursor.advance(); // Past 'defer' keyword.
    node->expression = parseExpressionList(ctxLhsExpr); // Allow '{' to be an initializer.
    return node;
}

Node Parser::parseUsing() {
    auto node = mem.New<UsingSyntax>(*cursor.pos);
    cursor.advance(); // Past 'using' keyword.
    node->expression = parseExpressionList(ctxRhsExpr); // Stop at '{'.
    node->statement = parseStatement();
    return node;
}

Node Parser::parseExpressionList(Ctx ctx) {
    if (auto node = parseExpression(ctx)) {
        CommaSeparatedSyntax *list{};
        while (is.Comma(cursor.pos)) {
            if (list == nullptr) {
                list = mem.New<CommaSeparatedSyntax>(node);
            }
            cursor.advance(); // Past ','
            if (auto next = parseExpression(ctx)) {
                list->nodes.append(next);
            }
        }
        if (list != nullptr) {
            return list;
        }
        return node;
    }
    return nullptr;
}

Node Parser::parseExpression(Ctx ctx) {
    if (((ctx & ctxTypeName) != 0) && is.OpenCurly(cursor.pos)) {
        // '{' is the first token in a typename context. Parse as object.
        traceln("parseExpression");
        Assert(0);
    }
    if (auto node = parseUnary(ctx)) {
        if ((ctx & ctxTypeName) == 0) {
            return parseInfix(node, ctx, Tok::Unknown);
        }
        return node;
    }
    return nullptr;
}

static auto precedenceOf(Pos op) {
    if (op->kind >= Tok::OrAssign && op->kind <= Tok::Assign) {
        return Tok::Assign;
    }
    if (op->kind == Tok::Question || op->keyword == Keyword::If) {
        return Tok::OrOr;
    } 
    if (is.kwInfix(op)) {
        return Tok::Exponentiation;
    }
    return op->kind;
}

Node Parser::parseInfix(Node lhs, Ctx ctx, Tok minPrec) {
    while (is.InfixOp(cursor.pos) && precedenceOf(cursor.pos) >= minPrec) {
        auto op = cursor.pos;
        if (op->kind == Tok::Question) {
            lhs = parseTernaryOp(lhs);
        } else if (op->keyword == Keyword::If) {
            if (cursor.hasNewLineBefore()) {
                // 'if' keyword is at statement level. Assume 'if' statement.
                break;
            }
            lhs = parseIfExpression(lhs);
        } else if (is.kwInfix(op)) {
            cursor.advance(); // Past 'as' ... 'in'.
            if (op->keyword == Keyword::NotIn || op->keyword == Keyword::In) {
                if (auto rhs = parseExpression(ctx)) {
                    lhs = mem.New<BinarySyntax>(lhs, *op, rhs);
                } else {
                    break;
                }
            } else if (auto rhs = parseExpression(ctxTypeName)) {
                lhs = mem.New<BinarySyntax>(lhs, *op, rhs);
            } else {
                break;
            }
        } else {
            cursor.advance(); // Past {op}.
            if (auto rhs = parseUnary(ctx)) {
                auto     prec = precedenceOf(op);
                auto   nextop = cursor.pos;
                auto nextprec = precedenceOf(nextop);
                while ((is.InfixOp(nextop) && nextprec > prec) ||
                       (is.RightAssociative(nextop) && nextprec == prec)) {
                    rhs = parseInfix(rhs, ctx, Tok(INT(prec) + 1));
                    if (nextop == cursor.pos) {
                        break;
                    }
                    nextop = cursor.pos;
                    nextprec = precedenceOf(nextop);
                }
                lhs = mem.New<BinarySyntax>(lhs, *op, rhs);
            } else {
                break;
            }
        }
    }
    return lhs;
}

Node Parser::parseTernaryOp(Node condition) {
    auto node = mem.New<TernarySyntax>(condition, *cursor.pos);
    cursor.advance(); // Past '?'.
    node->iftrue = parseExpression(ctxRhsExpr);
    if (is.Colon(cursor.pos)) {
        node->colon = cursor.pos;
        cursor.advance(); // Past ':'.
        if (auto expr = parseUnary(ctxRhsExpr)) {
            node->ifalse = parseInfix(expr, ctxRhsExpr, Tok::OrOr);
        }
    } else {
        syntax_error(cursor.pos, "expected ':', not %tok", cursor.pos);
    }
    return node;
}

Node Parser::parseIfExpression(Node iftrue) {
    auto node = mem.New<IfExpressionSyntax>(iftrue, *cursor.pos);
    cursor.advance(); // Past 'if'.
    node->condition = parseExpression(ctxRhsExpr);
    if (is.kwElse(cursor.pos)) {
        node->kwElse = cursor.pos;
        cursor.advance(); // Past 'else'
        if (auto expr = parseUnary(ctxRhsExpr)) {
            node->ifalse = parseInfix(expr, ctxRhsExpr, Tok::OrOr);
        }
    }
    return node;
}

Node Parser::parseUnary(Ctx ctx) {
    UnaryPrefixSyntax *node = nullptr, *last = nullptr;
    while (true) {
        UnaryPrefixSyntax *inner{};
        switch (cursor.pos->kind) {
            case Tok::Minus: {
                ((SourceToken*)cursor.pos)->kind = Tok::UnaryMinus;
                inner = mem.New<UnaryPrefixSyntax>(*cursor.pos);
            } break;
            case Tok::Plus: {
                ((SourceToken*)cursor.pos)->kind = Tok::UnaryPlus;
                inner = mem.New<UnaryPrefixSyntax>(*cursor.pos);
            } break;
            case Tok::Multiply: {
                ((SourceToken*)cursor.pos)->kind = Tok::Dereference;
                inner = mem.New<UnaryPrefixSyntax>(*cursor.pos);
            } break;
            case Tok::And: {
                ((SourceToken*)cursor.pos)->kind = Tok::AddressOf;
                inner = mem.New<UnaryPrefixSyntax>(*cursor.pos);
            } break;
            case Tok::Exponentiation: {
                ((SourceToken*)cursor.pos)->kind = Tok::Dereference;
                inner = mem.New<UnaryPrefixSyntax>(*cursor.pos);
                inner->expression = mem.New<UnaryPrefixSyntax>(*cursor.pos);
                inner = (UnaryPrefixSyntax*)inner->expression;
            } break;
            case Tok::AndAnd: {
                ((SourceToken*)cursor.pos)->kind = Tok::AddressOf;
                inner = mem.New<UnaryPrefixSyntax>(*cursor.pos);
                inner->expression = mem.New<UnaryPrefixSyntax>(*cursor.pos);
                inner = (UnaryPrefixSyntax*)inner->expression;
            } break;
            case Tok::MinusMinus:
            case Tok::PlusPlus:
            case Tok::LogicalNot:
            case Tok::BitwiseNot: {
                inner = mem.New<UnaryPrefixSyntax>(*cursor.pos);
            } break;
            default: switch (cursor.pos->keyword) {
                case Keyword::AlignOf:
                case Keyword::SizeOf:
                case Keyword::NameOf:
                case Keyword::TypeOf:
                case Keyword::New:
                case Keyword::Delete:
                case Keyword::Atomic: {
                    inner = mem.New<UnaryPrefixSyntax>(*cursor.pos);
                } break;
                case Keyword::Await: {
                    inner = mem.New<UnaryPrefixSyntax>(*cursor.pos);
                    if (auto fn = currentFunction.node) {
                        ++fn->awaits;
                    } else {
                        syntax_error(inner, "%kw outside of a function", inner->pos.keyword);
                    }
                } break;
            }
        }
        if (inner == nullptr) {
            break;
        }
        if (node == nullptr) {
            node = inner;
        } else if (last != nullptr) {
            last->expression = inner;
        }
        last = inner;
        cursor.advance(); // Past unary-prefix operator.
    }
    if (last != nullptr) {
        Assert(last->expression == nullptr);
        last->expression = parseUnarySuffix(ctx);
        if (is.kwWith(cursor.pos) && !cursor.hasNewLineBefore()) {
            last->kwWith = cursor.pos;
            cursor.advance(); // Past 'with' keyword.
            last->with = parseWith();
        }
        return node;
    }
    return parseUnarySuffix(ctx);
}

Node Parser::parseUnarySuffix(Ctx ctx) {
    if (auto node = parsePostfix(ctx)) {
        while (true) {
            switch (cursor.pos->kind) {
                case Tok::MinusMinus:
                case Tok::PlusPlus: if (!cursor.hasNewLineBefore()) {
                    node = mem.New<UnarySuffixSyntax>(node, *cursor.pos);
                } break;
                case Tok::Pointer:
                case Tok::Reference: {
                    node = mem.New<UnarySuffixSyntax>(node, *cursor.pos);
                } break;
                default: {
                    if ((ctx & ctxLhsExpr) != 0) {
                        if (auto syntax = is.PointerOrReferenceSyntax(node)) {
                            if (is.OpenCurly(cursor.pos) && !cursor.hasNewLineBefore()) {
                                node = mem.New<InitializerSyntax>(node, parseBraceArguments());
                            }
                        }
                    }
                    return node;
                }
            }
            cursor.advance(); // Past unary-suffix operator.
        }
    }
    return nullptr;
}

Node Parser::parsePostfix(Ctx ctx) {
    if (auto node = parsePrimary(ctx)) {
        while (true) {
            auto isFinished = false;
            switch (cursor.pos->kind) {
                case Tok::Dot: {
                    auto dot = mem.New<DotSyntax>(node, *cursor.pos);
                    cursor.advance(); // Past '.'
                    dot->rhs = parsePostfix(ctx);
                    node = dot;
                } break;
                case Tok::OpenParen: if (!cursor.hasNewLineBefore()) {
                    node = mem.New<CallSyntax>(node, parseCallArguments());
                    if (is.kwWith(cursor.pos) && (ctx & ctxNoWith) == 0 && !cursor.hasNewLineBefore()) {
                        auto call = (CallSyntax*)node;
                        call->kwWith = cursor.pos;
                        cursor.advance(); // Past 'with' keyword.
                        call->with = parseWith();
                    }
                } else {
                    isFinished = true;
                } break;
                case Tok::OpenBracket: if (!cursor.hasNewLineBefore()) {
                    node = mem.New<IndexSyntax>(node, parseIndexArguments());
                } else {
                    isFinished = true;
                } break;
                case Tok::OpenAngle: {
                    node = mem.New<TypeNameSyntax>(node, parseAngleArguments());
                } break;
                case Tok::OpenCurly: if (!cursor.hasNewLineBefore() && (ctx & ctxLhsExpr) != 0) {
                    node = mem.New<InitializerSyntax>(node, parseBraceArguments());
                } else {
                    isFinished = true;
                } break;
                default: {
                    isFinished = true;
                } break;
            }
            if (isFinished) {
                break;
            }
        }
        return node;
    }
    return nullptr;
}

ParenthesizedSyntax* Parser::parseCallArguments() {
    auto parenthesized = mem.New<ParenthesizedSyntax>(*cursor.pos);
    cursor.advance(); // Past '('.
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseParen(cursor.pos)) {
        auto pos = cursor.pos;
        Node node{};
        if (is.Identifier(cursor.pos) && is.Assign(cursor.next)) {
            auto nv = mem.New<NameValueSyntax>(mem.New<IdentifierSyntax>(*cursor.pos), *cursor.next);
            cursor.advance(); // Past identifier.
            cursor.advance(); // Past '='.
            nv->value = parseExpression(ctxLhsExpr);
            node = nv;
        } else if (node = parseExpression(ctxLhsExpr)) {
            // Do nothing.
        } else if (pos == cursor.pos) {
            cursor.advance(); // Past bad token.
        }
        if (node == nullptr) {
            // Do nothing.
        } else if (parenthesized->value == nullptr) {
            parenthesized->value = node;
        } else if (auto list = is.CommaSeparatedSyntax(parenthesized->value)) {
            list->nodes.append(node);
        } else {
            list = mem.New<CommaSeparatedSyntax>(parenthesized->value);
            list->nodes.append(node);
            parenthesized->value = list;
        }
        if (is.Comma(cursor.pos)) {
            cursor.advance(); // Past ','.
        }
    }
    if (is.CloseParen(cursor.pos)) {
        parenthesized->close = cursor.pos;
        cursor.advance(); // Past ')'.
    } else {
        syntax_error(parenthesized->pos, "unmatched %tok", &parenthesized->pos);
    }
    return parenthesized;
}

BracketedSyntax* Parser::parseIndexArguments() {
    auto bracketed = mem.New<BracketedSyntax>(*cursor.pos);
    cursor.advance(); // Past '['.
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseBracket(cursor.pos)) {
        auto pos = cursor.pos;
        Node node{};
        if (is.Identifier(cursor.pos) && is.Assign(cursor.next)) {
            auto nv = mem.New<NameValueSyntax>(mem.New<IdentifierSyntax>(*cursor.pos), *cursor.next);
            cursor.advance(); // Past identifier.
            cursor.advance(); // Past '='.
            nv->value = parseExpression(ctxLhsExpr);
            node = nv;
        } else if (node = parseExpression(ctxLhsExpr)) {
            // Do nothing.
        } else if (pos == cursor.pos) {
            cursor.advance(); // Past bad token.
        }
        if (node == nullptr) {
            // Do nothing.
        } else if (bracketed->value == nullptr) {
            bracketed->value = node;
        } else if (auto list = is.CommaSeparatedSyntax(bracketed->value)) {
            list->nodes.append(node);
        } else {
            list = mem.New<CommaSeparatedSyntax>(bracketed->value);
            list->nodes.append(node);
            bracketed->value = list;
        }
        if (is.Comma(cursor.pos)) {
            cursor.advance(); // Past ','.
        }
    }
    if (is.CloseBracket(cursor.pos)) {
        bracketed->close = cursor.pos;
        cursor.advance(); // Past ']'.
    } else {
        syntax_error(bracketed->pos, "unmatched %tok", &bracketed->pos);
    }
    return bracketed;
}

AngledSyntax* Parser::parseAngleArguments() {
    auto angled = mem.New<AngledSyntax>(*cursor.pos);
    cursor.advance(); // Past '<'.
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseAngle(cursor.pos)) {
        auto pos = cursor.pos;
        Node node{};
        if (is.Identifier(cursor.pos) && is.Assign(cursor.next)) {
            auto nv = mem.New<NameValueSyntax>(mem.New<IdentifierSyntax>(*cursor.pos), *cursor.next);
            cursor.advance(); // Past identifier.
            cursor.advance(); // Past '='.
            nv->value = parseExpression(ctxLhsExpr);
            node = nv;
        } else if (node = parseExpression(ctxLhsExpr)) {
            // Do nothing.
        } else if (pos == cursor.pos) {
            cursor.advance(); // Past bad token.
        }
        if (node == nullptr) {
            // Do nothing.
        } else if (angled->value == nullptr) {
            angled->value = node;
        } else if (auto list = is.CommaSeparatedSyntax(angled->value)) {
            list->nodes.append(node);
        } else {
            list = mem.New<CommaSeparatedSyntax>(angled->value);
            list->nodes.append(node);
            angled->value = list;
        }
    }
    if (is.CloseAngle(cursor.pos)) {
        angled->close = cursor.pos;
        cursor.advance(); // Past '>'.
    } else {
        syntax_error(angled->pos, "unmatched %tok", &angled->pos);
    }
    return angled;
}

BracedSyntax* Parser::parseBraceArguments() {
    auto braced = mem.New<BracedSyntax>(*cursor.pos);
    cursor.advance(); // Past '{'.
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseCurly(cursor.pos)) {
        auto pos = cursor.pos;
        Node node{};
        if (is.Identifier(cursor.pos) && is.Assign(cursor.next)) {
            auto nv = mem.New<NameValueSyntax>(mem.New<IdentifierSyntax>(*cursor.pos), *cursor.next);
            cursor.advance(); // Past identifier.
            cursor.advance(); // Past '='.
            nv->value = parseExpression(ctxLhsExpr);
            node = nv;
        } else if (is.Identifier(cursor.pos) && is.Colon(cursor.next)) {
            auto nv = mem.New<NameValueSyntax>(mem.New<IdentifierSyntax>(*cursor.pos), *cursor.next);
            cursor.advance(); // Past identifier.
            cursor.advance(); // Past ':'.
            nv->value = parseExpression(ctxTypeName);
            node = nv;
        } else if (node = parseExpression(ctxLhsExpr)) {
            // Do nothing.
        } else if (pos == cursor.pos) {
            cursor.advance(); // Past bad token.
        }
        if (node == nullptr) {
            // Do nothing.
        } else if (braced->value == nullptr) {
            braced->value = node;
        } else if (auto list = is.CommaSeparatedSyntax(braced->value)) {
            list->nodes.append(node);
        } else {
            list = mem.New<CommaSeparatedSyntax>(braced->value);
            list->nodes.append(node);
            braced->value = list;
        }
    }
    if (is.CloseCurly(cursor.pos)) {
        braced->close = cursor.pos;
        cursor.advance(); // Past '}'.
    } else {
        syntax_error(braced->pos, "unmatched %tok", &braced->pos);
    }
    return braced;
}

FunctionSyntax* Parser::parseWith() {
    auto kwWith = cursor.pos;
    if (is.OpenParen(cursor.pos)) {
        auto fn = mem.New<FunctionSyntax>(/* modifiers = */ nullptr, *cursor.pos);
        ((SourceToken&)fn->pos).keyword = Keyword::Lambda; // Sneaky!
        currentFunction = { &currentFunction, fn };
        fn->parameters = (ParenthesizedSyntax*)parseFunctionParameters();
        if (is.Colon(cursor.pos) || is.DashArrow(cursor.pos)) {
            fn->fnreturnOp = cursor.pos;
            fn->fnreturn = parseStructureSupers();
        }
        if (is.OpenCurly(cursor.pos)) {
            fn->body = parseBlock(nullptr);
        } else if (is.SemiColon(cursor.pos)) {
            fn->body = mem.New<EmptySyntax>(*cursor.pos);
            cursor.advance(); // Past ';'
        } else if (is.Assign(cursor.pos) || is.AssignArrow(cursor.pos)) {
            fn->bodyOp = cursor.pos;
            cursor.advance(); // Past '=' | '=>'.
            fn->body = parseStatement();
        } else {
            syntax_error(cursor.pos, "expected '{', ';', '=' or '=>', not %tok", cursor.pos);
        }
        if (currentFunction.prev) {
            currentFunction = *currentFunction.prev;
        }
        return fn;
    } 
    if (is.OpenCurly(cursor.pos)) {
        auto fn = mem.New<FunctionSyntax>(/* modifiers = */ nullptr, *cursor.pos);
        ((SourceToken&)fn->pos).keyword = Keyword::Lambda; // Sneaky!
        currentFunction = { &currentFunction, fn };
        fn->body = parseBlock(nullptr);
        if (currentFunction.prev) {
            currentFunction = *currentFunction.prev;
        }
        return fn;
    } 
    syntax_error(cursor.pos, "expected '(',  or '{' after %tok, not %tok", kwWith, cursor.pos);
    return nullptr;
}

Node Parser::parsePrimary(Ctx ctx) {
    auto modifiers = parseModifiers();
    if (modifiers != nullptr) {
        if (is.UDT(cursor.pos)) {
            return parseUDT(modifiers);
        }
        if (is.Identifier(cursor.pos)) {
            return parseVariable(modifiers, ctx);
        }
        if (is.OpenParen(cursor.pos)) {
            return parseMultiVariable(modifiers);
        }
        syntax_error(modifiers, "dangling modifiers: expected a UDT or variable declaration after modifiers");
        return modifiers;
    }
    if (is.UDT(cursor.pos)) {
        return parseUDT(modifiers);
    }
    if (is.Primitive(cursor.pos)) {
        auto node = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past 'Void' ... 'UInt64x8'.
        return node;
    }
    switch (cursor.pos->kind) {
        case Tok::OpenParen:
            return parseParenthesized(ctx);
        case Tok::OpenBracket:
            return parseBracketed(ctx);
        case Tok::OpenCurly:
            return parseCurlyBraced(ctx);
        case Tok::Ellipsis:
            return parseRest();
        case Tok::Decimal:
            return parseDecimal();
        case Tok::Hexadecimal:
            return parseHexadecimal();
        case Tok::Binary:
            return parseBinary();
        case Tok::Octal:
            return parseOctal();
        case Tok::Float:
            return parseFloat();
        case Tok::DecimalFloat:
            return parseDecimalFloat();
        case Tok::HexadecimalFloat:
            return parseHexadecimalFloat();
        case Tok::BinaryFloat:
            return parseBinaryFloat();
        case Tok::OctalFloat:
            return parseOctalFloat();
        case Tok::SingleQuote:
        case Tok::DoubleQuote:
            return parseQuoted();
    }
    switch (cursor.pos->keyword) {
        case Keyword::True:
        case Keyword::False: {
            auto node = mem.New<BooleanSyntax>(*cursor.pos);
            cursor.advance(); // Past 'true' | 'false'.
            return node;
        }
        case Keyword::This:
        case Keyword::Super: {
            auto node = mem.New<IdentifierSyntax>(*cursor.pos);
            cursor.advance(); // Past 'this' | 'super'.
            return node;
        }
        case Keyword::Null: {
            auto node = mem.New<NullSyntax>(*cursor.pos);
            cursor.advance(); // Past 'null'.
            return node;
        }
        case Keyword::void_: {
            auto node = mem.New<VoidSyntax>(*cursor.pos);
            cursor.advance(); // Past 'void'.
            return node;
        }
    }
    if (is.Identifier(cursor.pos)) {
        if (is.ColonAssign(cursor.next)) {
            return parseVariable(nullptr, ctx);
        }
        auto node = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past identifier.
        return node;
    }
    if (is.CompilerKeyword(cursor.pos)) {
        auto node = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past compiler keyword.
        return node;
    }
    syntax_error(cursor.pos, "expected an expression, not %tok", cursor.pos);
    return nullptr;
}

Node Parser::parseParenthesized(Ctx ctx) {
    auto parenthesized = mem.New<ParenthesizedSyntax>(*cursor.pos);
    cursor.advance(); // Past '('.
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseParen(cursor.pos)) {
        if (auto node = parseExpression((ctx & ctxTypeName) != 0 ? ctxTypeName : ctxLhsExpr)) {
            if (parenthesized->value == nullptr) {
                parenthesized->value = node;
            } else if (auto list = is.CommaSeparatedSyntax(parenthesized->value)) {
                list->nodes.append(node);
            } else {
                list = mem.New<CommaSeparatedSyntax>(parenthesized->value);
                list->nodes.append(node);
                parenthesized->value = list;
            }
        } else {
            cursor.advance(); // Past bad token.
        }
        if (is.Comma(cursor.pos)) {
            cursor.advance(); // Past ','.
        }
    }
    if (is.CloseParen(cursor.pos)) {
        parenthesized->close = cursor.pos;
        cursor.advance(); // Past ')'.
    } else {
        syntax_error(parenthesized->pos, "unmatched %tok", &parenthesized->pos);
    }
    return parenthesized;
}

Node Parser::parseBracketed(Ctx ctx) {
    auto bracketed = mem.New<BracketedSyntax>(*cursor.pos);
    cursor.advance(); // Past '['.
    while (is.NotEndOfFile(cursor.pos) && is.NotCloseBracket(cursor.pos)) {
        if (auto node = parseExpression((ctx & ctxTypeName) != 0 ? ctxTypeName : ctxLhsExpr)) {
            if (bracketed->value == nullptr) {
                bracketed->value = node;
            } else if (auto list = is.CommaSeparatedSyntax(bracketed->value)) {
                list->nodes.append(node);
            } else {
                list = mem.New<CommaSeparatedSyntax>(bracketed->value);
                list->nodes.append(node);
                bracketed->value = list;
            }
        } else {
            cursor.advance(); // Past bad token.
        }
        if (is.Comma(cursor.pos)) {
            cursor.advance(); // Past ','.
        }
    }
    if (is.CloseBracket(cursor.pos)) {
        bracketed->close = cursor.pos;
        cursor.advance(); // Past ']'.
    } else {
        syntax_error(bracketed->pos, "unmatched %tok", &bracketed->pos);
    }
    return bracketed;
}

Node Parser::parseCurlyBraced(Ctx ctx) {
    if (!cursor.hasNewLineBefore() && (ctx & ctxLhsExpr) != 0) {
        return parseBraceArguments();
    }
    syntax_error(cursor.pos, "unexpected %tok", &cursor.pos);
    return nullptr;
}

Node Parser::parseRest() {
    auto node = mem.New<RestSyntax>(*cursor.pos);
    cursor.advance(); // Past '...'
    if (is.Identifier(cursor.pos)) {
        node->name = mem.New<IdentifierSyntax>(*cursor.pos);
        cursor.advance(); // Past identifier.
    }
    return node;
}
} // namespace exy
