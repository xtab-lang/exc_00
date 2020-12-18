//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-05
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef PARSER_H
#define PARSER_H

#define err(token, msg, ...) print_error("Parser", token, msg, __VA_ARGS__)

#include "syn_cursor.h"
#include "syntax.h"

namespace exy {
//--Begin forward declarations
using  Node = SyntaxNode*;
using Nodes = List<SyntaxNode*>&;
//----End forward declarations
struct Parser {
    Cursor cur;
    Mem   &mem;

    Parser(const List<SourceToken> &tokens, Mem &mem);
    void dispose();

    void parseFile(SyntaxFile&);
    void setModule(SyntaxFile&, SyntaxModule*);
    void createModule(SyntaxFile&);

    Node parseStatement();

    Node parseModule(Node modifiers);

    Node parseImport(Nodes list);
    void parseImport(SyntaxImportOrExport*);
    Node parseImportAlias(SyntaxImportOrExport*);
    Node parseImportFrom(SyntaxImportOrExport*);

    Node parseLet(Node modifiers);

    Node parseExpression();
    Node parseExpression(Node modifiers);

    Node parseUnaryPrefix(Node modifiers);
    Node parseUnarySuffix(Node modifiers);

    Node parseDotExpression(Node modifiers);

    Node parseArgumentizedExpression(Node modifiers);
    Node parseCallExpression(Node name);
    Node parseIndexExpression(Node name);
    Node parseTypeNameExpression(Node name);
    Node parseObjectExpression(Node name);

    void parseArguments(Nodes list);
    Node parseArgument();

    Node parseTerm(Node modifiers);
    Node parseLiteral(Node modifiers);
    Node parseVariable(Node modifiers);
    Node parseIdentifier(Node modifiers);

    Node parseFullName(Node modifiers);
    Node parseModifiers();
    Node empty(Token pos, Node modifiers);

    Node parseDecimal(Node modifiers);
    Node parseHexadecimal(Node modifiers);
    Node parseBinary(Node modifiers);
    Node parseOctal(Node modifiers);
    Node parseFloat(Node modifiers);
};
} // namespace exy

#endif // SYNTAX_H