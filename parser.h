#pragma once

#include "syntax.h"

namespace exy {
struct Parser {
	using  Pos  = const SourceToken*;
	using  Node = SyntaxNode*;
	using Nodes = List<Node>;

	struct Cursor {
		Pos start;
		Pos pos;
		Pos end;
		Pos next{};
		Pos prev{};

		Cursor(Pos start, Pos end) : start(start), pos(start), end(end) {}

		Pos advance();
		Pos skipWhiteSpace();
		bool hasNewLineAfter(Pos = nullptr);
		bool hasNewLineBefore(Pos = nullptr);

	} cursor;
	SyntaxFile &file;
	Mem        &mem;

	Parser(SyntaxFile&);
	void dispose();

	void run();
private:
	enum Ctx {
		ctxLhsExpr  = 0x01, // Assume that '{' starts an initializer.
		ctxRhsExpr  = 0x02, // Stop at '{'.
		ctxTypeName = 0x04, // Stop at '{' (unless it is the first token encountered) or '='.
		ctxNoIf     = 0x08, // Stop at 'if'.
		ctxNoIn     = 0x10, // Stop at 'in.
	};

	Node parseStatement();

	Node parseModule();
	Node parseImport();
	void parseImportAlias(ImportSyntax*);
	void parseImportSource(ImportSyntax*);
	Node parseImportList(Node first);
	Node parseModuleName();
	Node parseDotModuleName(Node lhs);

	Node parseDefine(Node modifiers);

	Node parseExternsBlock(Node modifiers);

	Node parseStructure(Node modifiers);
	Node parseStructureName();
	Node parseStructureParameters();
	Node parseStructureAttributes();
	Node parseStructureSupers();

	Node parseFunction(Node modifiers);
	void parseFunctionName(FunctionSyntax*);
	void parseUrlName(FunctionSyntax*);
	void parseFunctionOperatorName(FunctionSyntax*);
	Node parseFunctionParameters();
	Node parseFunctionParameter(Node modifiers);

	Node parseModifiers();
	Node parseVariable(Node modifiers, Ctx = ctxLhsExpr);
	Node parseMultiVariable(Node modifiers);
	Node parseBlock(Node modifiers);
	Node parseUDT(Node modifiers);

	Node parseTextBlock();
	Node parseCodeBlock();

	Node parseIf(Node modifiers);
	Node parseSwitch();
	Node parseCaseList(SwitchSyntax*);
	Node parseCaseCondition(SwitchSyntax*);
	Node parseCaseConditionList(SwitchSyntax*, Node first);

	Node parseAssert();
	Node parseThrow();
	Node parseReturn();
	Node parseBreakOrContinue();

	Node parseFor();
	Node parseAwaitFor();
	Node parseForElse(ForInSyntax*);
	Node parseForElse(ForSyntax*);
	Node parseWhile();
	Node parseDoWhile();
	Node parseDefer();
	Node parseUsing();

	Node parseExpressionList(Ctx);
	Node parseExpression(Ctx);
	Node parseInfix(Node, Ctx, Tok minPrec);
	Node parseTernaryOp(Node);
	Node parseIfExpression(Node);
	Node parseUnary(Ctx);
	Node parseUnarySuffix(Ctx);
	Node parsePostfix(Ctx);
	ParenthesizedSyntax* parseCallArguments();
	BracketedSyntax* parseIndexArguments();
	AngledSyntax* parseAngleArguments();
	BracedSyntax* parseBraceArguments();
	Node parsePrimary(Ctx);
	Node parseParenthesized(Ctx);
	Node parseBracketed(Ctx);
	Node parseRest();

	Node parseQuoted();

	// parse_number.cpp
	struct Num {
		enum Bits { NoBits, Bits8 = 8, Bits16 = 16, Bits32 = 32, Bits64 = 64 };
		String value{};
		Bits   width{};
		bool   isUnsigned{};
	};
	Node parseDecimal();
	Node parseHexadecimal();
	Node parseBinary();
	Node parseOctal();
	Node parseFloat();
	Node parseDecimalFloat();
	Node parseHexadecimalFloat();
	Node parseBinaryFloat();
	Node parseOctalFloat();
};
} // namespace exy
