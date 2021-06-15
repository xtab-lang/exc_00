#include "pch.h"
#include "tokenizer.h"

#include "token_processor.h"
#include "src.h"

#define err(pos, msg, ...) compiler_error("Tokenizer", msg, __VA_ARGS__)

namespace exy {
struct SourceCharReader {
	const CHAR *pos;
	const CHAR *end;
	INT         line = 1;
	INT         col  = 1;

	SourceCharReader(const String &source) : pos(source.start()), end(source.end()) {}

	auto read() {
		List<SourceChar> list{};
		for (; pos < end; pos++) {
			if (*pos == '\r' && pos + 1 < end && pos[1] == '\n') {
				list.place(pos, line, col);
				++line;
				col = 1;
				++pos; // Now at LF. The {pos}++ above moves past '\n'.
			} else if (*pos == '\n') {
				list.place(pos, line, col);
				++line;
				col = 1;
			} else {
				list.place(pos, line, col);
				movePos();
				++col;
			}
		}
		list.place(end, line, col);
		return list;
	}

	void movePos() {
		auto len = Tokenizer::lengthOf(*pos);
		pos += len - 1;
	}
};
//----------------------------------------------------------
struct SourceCharStream {
	const SourceFile &file;
	const SourceChar *start;
	const SourceChar *pos;
	const SourceChar *end;

	SourceCharStream(const SourceFile &file, const List<SourceChar> &list) : file(file),
		start(list.start()), pos(list.start()), end(list.end()) {}

	void validate() {
		for (; pos < end; ++pos ) {
			if (!posIsValidUtf8()) {
				SourcePos token{ file, *pos, pos[1] };
				err(token, "invalid UTF8 character");
			}
		}
		pos = start;
	}

	bool posIsValidUtf8() {
		switch (Tokenizer::lengthOf(*pos->text)) {
			case 2: {
				if (pos + 1 < end) {
					return (INT(*pos[1].text) & 0xC0) == 0x80;
				}
				return false;
			} break;
			case 3: {
				if (pos + 2 < end) {
					return (INT(*pos[1].text) & 0xC0) == 0x80 &&
						(INT(*pos[2].text) & 0xC0) == 0x80;
				}
				return false;
			} break;
			case 4: {
				if (pos + 3 < end) {
					return (INT(*pos[1].text) & 0xC0) == 0x80 &&
						(INT(*pos[2].text) & 0xC0) == 0x80 &&
						(INT(*pos[3].text) & 0xC0) == 0x80;
				}
				return false;
			} break;
		}
		return true;
	}
};
//----------------------------------------------------------
Tokenizer::Tokenizer(SourceFile &file) : file(file), tokens(file.tokens) {}

void Tokenizer::run() {
	SourceCharReader reader{ file.source };
	auto src = reader.read();
	file.lines = reader.line;
	file.characters = src.length;
	SourceCharStream stream{ file, src };
	stream.validate();
	read(stream);
	src.dispose();
	TokenProcessor processor{ file };
	processor.run();
	processor.dispose();
}

INT Tokenizer::lengthOf(const CHAR src) {
	const auto ch = INT(src);
	if ((ch & 0x80) == 0) { // 1 B UTF8 character.
		return 1;
	}
	if ((ch & 0xE0) == 0xC0) { // 2 B UTF8 character.
		return 2;
	}
	if ((ch & 0xF0) == 0xE0) { // 3 B UTF8 character.
		return 3;
	}
	if ((ch & 0xF8) == 0xF0) { // 4 B UTF8 character.
		return 4;
	} // Else not utf8.
	return 1;
}

void Tokenizer::read(Stream stream) {
	pos = stream.pos;
	while (stream.pos < stream.end) {
		pos = stream.pos++;
		if (isAlpha(pos)) {
			readText(stream);
		} else {
			readPunctuation(stream);
		}
	}
	Assert(stream.pos == stream.end);
	take(stream, Tok::EndOfFile);
}

void Tokenizer::readText(Stream stream) {
	while (stream.pos < stream.end) {
		if (!isAlphaNumeric(stream.pos)) {
			break; // Stop.
		}
		++stream.pos;
	}
	take(stream, Tok::Text);
}

void Tokenizer::readPunctuation(Stream stream) {
	// {stream.pos} is now past {pos}.
	switch (auto ch = *pos->text) {
		case '\r': {
			if (stream.pos->text - pos->text == 2) {
				readWhiteSpace(stream, Tok::NewLine); // CR LF
			} else { // CR
				take(stream, Tok::Text);
			}
		} break;
		case '\n': {
			readWhiteSpace(stream, Tok::NewLine);
		} break;
		case ' ':
		case '\t': {
			readWhiteSpace(stream, Tok::Space);
		} break;
		case ',': {
			take(stream, Tok::Comma);
		} break;
		case ';': {
			take(stream, Tok::SemiColon);
		} break;
		case '\\': {
			take(stream, Tok::BackSlash);
		} break;
		case '\'': {
			take(stream, Tok::SingleQuote);
		} break;
		case '"': {
			take(stream, Tok::DoubleQuote);
		} break;
		case '#': if (*stream.pos->text == '#') {
			++stream.pos; // past 2nd '#'
			take(stream, Tok::Hash); // take '##'
		} else if (*stream.pos->text == '(') {
			++stream.pos; // past '('
			take(stream, Tok::HashOpenParen); // take '#('
		} else if (*stream.pos->text == '[') {
			++stream.pos; // past '('
			take(stream, Tok::HashOpenBracket); // take '#['
		} else {
			take(stream, Tok::Hash); // take '#'
		} break;
		case '@': if (*stream.pos->text == '@') {
			++stream.pos; // past 2nd '@'
			take(stream, Tok::AtAt); // take '@@'
		} else {
			take(stream, Tok::At); // take '@'
		} break;
		case '.': if (*stream.pos->text == '.') {
			++stream.pos; // past 2nd '.'
			if (stream.pos + 1 < stream.end && *stream.pos->text == '.') {
				++stream.pos; // past 3rd '.'
				take(stream, Tok::Ellipsis); // take '...'
			} else {
				take(stream, Tok::DotDot); // take '..'
			}
		} else {
			take(stream, Tok::Dot); // take '.'
		} break;
		case ':': if (*stream.pos->text == ':') {
			++stream.pos; // past 2nd ':'
			take(stream, Tok::ColonColon); // take '::'
		} else if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::ColonAssign); // take ':='
		} else {
			take(stream, Tok::Colon); // take ':'
		} break;
		case 'w': if (*stream.pos->text == '\'') {
			++stream.pos; // past '
			take(stream, Tok::WideSingleQuote); // take "w'"
		} else if (*stream.pos->text == '"') {
			++stream.pos; // past "
			take(stream, Tok::WideDoubleQuote); // take 'w"'
		} else {
			readText(stream);
		} break;
		case 'r': if (*stream.pos->text == '\'') {
			++stream.pos; // past '
			take(stream, Tok::RawSingleQuote); // take "r'"
		} else if (*stream.pos->text == '"') {
			++stream.pos; // past "
			take(stream, Tok::RawDoubleQuote); // take 'r"'
		} else {
			readText(stream);
		} break;
		case '/': if (*stream.pos->text == '/') {
			++stream.pos; // past 2nd '/'
			take(stream, Tok::OpenSingleLineComment); // take '//'
		} else if (*stream.pos->text == '*') {
			++stream.pos; // past '*'
			take(stream, Tok::OpenMultiLineComment); // take '/*'
		} else if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::DivideAssign); // take '/='
		} else {
			take(stream, Tok::Divide); // take '/'
		} break;
		case '*': if (*stream.pos->text == '/') {
			++stream.pos; // past '/'
			take(stream, Tok::CloseMultiLineComment); // take '*/'
		} else if (*stream.pos->text == '*') {
			++stream.pos; // past 2nd '*'
			if (stream.pos + 1 < stream.end && *stream.pos->text == '=') {
				++stream.pos; // past '='
				take(stream, Tok::ExponentiationAssign); // take '**='
			} else {
				take(stream, Tok::Exponentiation); // take '**'
			}
		} else if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::MultiplyAssign); // take '*='
		} else {
			take(stream, Tok::Multiply); // take '*'
		} break;
		case '~': if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::BitwiseNotAssign); // take '~='
		} else {
			take(stream, Tok::BitwiseNot); // take '~'
		} break;
		case '{': {
			take(stream, Tok::OpenCurly);
		} break;
		case '}': if (*stream.pos->text == '#') {
			++stream.pos; // past '#'
			take(stream, Tok::CloseCurlyHash); // take '}#'
		} else {
			take(stream, Tok::CloseCurly);
		} break;
		case '(': {
			take(stream, Tok::OpenParen);
		} break;
		case ')': {
			take(stream, Tok::CloseParen);
		} break;
		case '[': {
			take(stream, Tok::OpenBracket);
		} break;
		case ']': {
			take(stream, Tok::CloseBracket);
		} break;
		case '|': if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::OrAssign);
		} else if (*stream.pos->text == '|') {
			++stream.pos; // past 2nd '|'
			take(stream, Tok::OrOr);
		} else {
			take(stream, Tok::Or);
		} break;
		case '^': if (*stream.pos->text == '=') {
			++stream.pos; // Past '='.
			take(stream, Tok::XOrAssign);
		} else {
			take(stream, Tok::XOr);
		} break;
		case '&': if (*stream.pos->text == '=') {
			++stream.pos; // Past '='.
			take(stream, Tok::AndAssign);
		} else if (*stream.pos->text == '|') {
			++stream.pos; // Past 2nd '&'.
			take(stream, Tok::AndAnd);
		} else {
			take(stream, Tok::And);
		} break;
		case '?': if (*stream.pos->text == '?') {
			++stream.pos; // past 2nd '?'
			if (stream.pos + 1 < stream.end && *stream.pos->text == '=') {
				++stream.pos; // past '='
				take(stream, Tok::QuestionQuestionAssign); // take '??='
			} else {
				take(stream, Tok::QuestionQuestion); // take '??'
			}
		} else {
			take(stream, Tok::Question); // take '?'
		} break;
		case '!': if (*stream.pos->text == '=') {
			++stream.pos; // Past '='.
			if (stream.pos + 1 < stream.end && *stream.pos->text == '=') {
				++stream.pos; // Past 2nd '='.
				take(stream, Tok::NotEquivalent); // Take '!=='
			} else {
				take(stream, Tok::NotEqual); // Take '!='
			}
		} else if (*stream.pos->text == '<') {
			++stream.pos; // Past '<'.
			take(stream, Tok::GreaterOrEqual); // take '!<' as '>='
		} else if (*stream.pos->text == '>') {
			++stream.pos; // past '>'
			take(stream, Tok::LessOrEqual); // take '!>' as '<='
		} else {
			take(stream, Tok::LogicalNot);
		} break;
		case '=': if (*stream.pos->text == '=') {
			++stream.pos; // past 2nd '='
			if (stream.pos + 1 < stream.end && *stream.pos->text == '=') {
				++stream.pos; // past 3rd '='
				take(stream, Tok::Equivalent);
			} else if (*stream.pos->text == '>') {
				++stream.pos; // past '>'
				take(stream, Tok::AssignArrow);
			} else {
				take(stream, Tok::Equal);
			}
		} else {
			take(stream, Tok::Assign);
		} break;
		case '<': if (*stream.pos->text == '<') {
			++stream.pos; // past 2nd '<'
			if (stream.pos + 1 < stream.end && *stream.pos->text == '=') {
				++stream.pos; // past '='
				take(stream, Tok::LeftShiftAssign); // take '<<='
			} else {
				take(stream, Tok::LeftShift); // take '<<'
			}
		} else if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::LessOrEqual); // take '>>='
		} else {
			take(stream, Tok::Less); // take '<'
		} break;
		case '>': if (*stream.pos->text == '>') {
			++stream.pos; // past 2nd '>'
			if (stream.pos + 1 < stream.end && *stream.pos->text == '>') {
				++stream.pos; // past 3rd '>'
				if (stream.pos + 1 < stream.end && *stream.pos->text == '=') {
					++stream.pos; // past '='
					take(stream, Tok::UnsignedRightShiftAssign); // take '>>>='
				} else {
					take(stream, Tok::UnsignedRightShift); // take '>>>'
				}
			} else {
				take(stream, Tok::RightShift); // take '>>'
			}
		} else if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::GreaterOrEqual); // take '>>='
		} else {
			take(stream, Tok::Greater); // take '>'
		} break;
		case '-': if (*stream.pos->text == '-') {
			++stream.pos; // past 2nd '-'
			take(stream, Tok::MinusMinus);
		} else if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::MinusAssign);
		} else if (*stream.pos->text == '>') {
			++stream.pos; // past '>'
			take(stream, Tok::DashArrow);
		} else {
			take(stream, Tok::Minus);
		} break;
		case '+': if (*stream.pos->text == '+') {
			++stream.pos; // past 2nd '+'
			take(stream, Tok::PlusPlus);
		} else if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::PlusAssign);
		} else {
			take(stream, Tok::Plus);
		} break;
		case '%': if (*stream.pos->text == '%') {
			++stream.pos; // past 2nd '%'
			take(stream, Tok::DivRem);
		} else if (*stream.pos->text == '=') {
			++stream.pos; // past '='
			take(stream, Tok::RemainderAssign);
		} else {
			take(stream, Tok::Remainder);
		} break;
		default: if (isaDigit(pos)) {
			readNumber(stream);
		} else {
			readText(stream);
		} break;
	}
}

void Tokenizer::readWhiteSpace(Stream stream, Tok tok) {
	auto newLines = tok == Tok::NewLine ? 1 : 0;
	auto finished = false;
	while (stream.pos < stream.end && !finished) {
		switch (auto ch = *stream.pos->text) {
			case ' ':
			case '\t': {
			} break;
			case '\r': {
				auto next = stream.pos + 1;
				if (next->text - stream.pos->text == 2) {
					++newLines; // CR LF
				} else { // CR
					return take(stream, Tok::Text);
				}
			} break;
			case '\n': {
				++newLines;
			} break;
			default: {
				finished = true;
			} break;
		}
		if (!finished) {
			++stream.pos;
		}
	}
	if (newLines > 0) {
		take(stream, Tok::NewLine);
	} else {
		take(stream, Tok::Space);
	}
}

void Tokenizer::readNumber(Stream stream) {
	if (tryDecOrFloat(stream)) {
		return;
	}
	if (tryHex(stream)) {
		return;
	}
	if (tryBin(stream)) {
		return;
	}
	if (tryOct(stream)) {
		return;
	}
	stream.pos = pos;
	readText(stream);
}

bool Tokenizer::tryDecOrFloat(Stream stream) {
	// dec [dec | '_']+ intsfx | exp [fltsfx] | fltsfx
	// dec [dec | '_']+ '.' dec [dec | '_']+ [exp] [fltsfx]
	stream.pos = pos; // set {stream.pos}
	for (; stream.pos < stream.end; ++stream.pos) {
		if (!isaDigitOrBlank(stream.pos)) {
			break;
		}
	}
	if (*stream.pos->text == '.') {
		return continueDecimalFromDot(stream);
	}
	if (isExponent(stream.pos)) {
		return tryExponent(stream, nullptr);
	}
	if (isIntSuffix(stream.pos)) {
		return tryIntSuffix(stream, Tok::Decimal);
	}
	if (isFloatSuffix(stream.pos)) {
		return tryFloatSuffix(stream, Tok::DecimalFloat);
	}
	if (!isAlpha(stream.pos)) {
		take(stream, Tok::Decimal);
		return true;
	}
	return false;
}

bool Tokenizer::tryHex(Stream stream) {
	stream.pos = pos; // set {stream.pos}
	if (isZero(stream.pos)) {
		// '0' 'x' | 'X' hex [hex | '_']+ intsfx | hexfltsfx
		++stream.pos;
		if (isaHexPrefix(stream.pos)) {
			++stream.pos;
			if (isaHex(stream.pos)) {
				for (++stream.pos; stream.pos < stream.end; ++stream.pos) {
					if (!isaHexOrBlank(stream.pos)) {
						break;
					}
				}
				if (isIntSuffix(stream.pos)) {
					return tryIntSuffix(stream, Tok::Hexadecimal);
				}
				if (isHexFloatSuffix(stream.pos)) {
					return tryFloatSuffix(stream, Tok::HexadecimalFloat);
				}
				if (!isAlpha(stream.pos)) {
					take(stream, Tok::Hexadecimal);
					return true;
				}
			}
		}
	}
	// hex [hex | '_']+ 'h' | 'H' intsfx | fltsfx
	stream.pos = pos; // set {stream.pos}
	if (isaHex(stream.pos)) {
		for (++stream.pos; stream.pos < stream.end; ++stream.pos) {
			if (!isaHexOrBlank(stream.pos)) {
				break;
			}
		}
		if (isHexSuffix(stream.pos)) {
			++stream.pos;
			if (isIntSuffix(stream.pos)) {
				return tryIntSuffix(stream, Tok::Hexadecimal);
			}
			if (isHexFloatSuffix(stream.pos)) {
				return tryFloatSuffix(stream, Tok::HexadecimalFloat);
			}
			if (!isAlpha(stream.pos)) {
				take(stream, Tok::Hexadecimal);
				return true;
			}
		}
	}
	return false;
}

bool Tokenizer::tryBin(Stream stream) {
	stream.pos = pos; // set {stream.pos}
	if (isZero(stream.pos)) {
		// '0' 'b' | 'B' bin [bin | '_']+ intsfx | fltsfx
		++stream.pos;
		if (isBinSuffix(stream.pos)) {
			++stream.pos;
			if (isaBin(stream.pos)) {
				for (++stream.pos; stream.pos < stream.end; ++stream.pos) {
					if (!isaBinOrBlank(stream.pos)) {
						break;
					}
				}
				if (isIntSuffix(stream.pos)) {
					return tryIntSuffix(stream, Tok::Binary);
				}
				if (isFloatSuffix(stream.pos)) {
					return tryFloatSuffix(stream, Tok::BinaryFloat);
				}
				if (!isAlpha(stream.pos)) {
					take(stream, Tok::Binary);
					return true;
				}
			}
		}
	}
	// bin [bin | '_']+ 'b' | 'B' intsfx | fltsfx
	stream.pos = pos; // set {stream.pos}
	if (isaBin(stream.pos)) {
		for (++stream.pos; stream.pos < stream.end; ++stream.pos) {
			if (!isaBinOrBlank(stream.pos)) {
				break;
			}
		}
		if (isBinSuffix(stream.pos)) {
			++stream.pos;
			if (isIntSuffix(stream.pos)) {
				return tryIntSuffix(stream, Tok::Binary);
			}
			if (isFloatSuffix(stream.pos)) {
				return tryFloatSuffix(stream, Tok::BinaryFloat);
			}
			if (!isAlpha(stream.pos)) {
				take(stream, Tok::Binary);
				return true;
			}
		}
	}
	return false;
}

bool Tokenizer::tryOct(Stream stream) {
	stream.pos = pos; // set {stream.pos}
	if (isZero(stream.pos)) {
		// '0' 'o' | 'O' oct [oct | '_']+ intsfx | fltsfx
		++stream.pos;
		if (isOctSuffix(stream.pos)) {
			++stream.pos;
			if (isanOct(stream.pos)) {
				for (++stream.pos; stream.pos < stream.end; ++stream.pos) {
					if (!isanOctOrBlank(stream.pos)) {
						break;
					}
				}
				if (isIntSuffix(stream.pos)) {
					return tryIntSuffix(stream, Tok::Octal);
				}
				if (isFloatSuffix(stream.pos)) {
					return tryFloatSuffix(stream, Tok::OctalFloat);
				}
				if (!isAlpha(stream.pos)) {
					take(stream, Tok::Octal);
					return true;
				}
			}
		}
	}
	// oct [oct | '_']+ 'o' | 'O' intsfx | fltsfx
	stream.pos = pos; // set {stream.pos}
	if (isaBin(stream.pos)) {
		for (++stream.pos; stream.pos < stream.end; ++stream.pos) {
			if (!isanOctOrBlank(stream.pos)) {
				break;
			}
		}
		if (isOctSuffix(stream.pos)) {
			++stream.pos;
			if (isIntSuffix(stream.pos)) {
				return tryIntSuffix(stream, Tok::Octal);
			}
			if (isFloatSuffix(stream.pos)) {
				return tryFloatSuffix(stream, Tok::OctalFloat);
			}
			if (!isAlpha(stream.pos)) {
				take(stream, Tok::Octal);
				return true;
			}
		}
	}
	return false;
}

bool Tokenizer::continueDecimalFromDot(Stream stream) {
	auto dot = stream.pos++; // past '.'
	if (isaDigit(stream.pos)) {
		for (++stream.pos; stream.pos < stream.end; ++stream.pos) {
			if (!isaDigitOrBlank(stream.pos)) {
				break;
			}
		}
		if (isExponent(stream.pos)) {
			if (tryExponent(stream, dot)) {
				return true;
			}
		} else if (isFloatSuffix(stream.pos)) {
			if (tryFloatSuffix(stream, Tok::Float)) {
				return true;
			}
		} else if (!isAlpha(stream.pos)) {
			take(stream, Tok::Float);
			return true;
		}
	}
	stream.pos = dot; // rewind to '.'
	take(stream, Tok::Decimal);
	return true;
}

bool Tokenizer::tryExponent(Stream stream, Pos dot) {
	// dec [dec | '_']+ 'e' | 'E' ['-' | '+'] dec [dec | '_']+ [fltsfx]
	// dec [dec | '_']+ '.' dec [dec | '_']+ 'e' | 'E' ['-' | '+'] dec [dec | '_']+ [fltsfx]
	stream.pos++; // past 'e' | 'E'
	if (isSign(stream.pos)) {
		stream.pos++; // past '-' | '+'
	}
	if (isaDigit(stream.pos)) {
		for (++stream.pos; stream.pos < stream.end; ++stream.pos) {
			if (!isaDigitOrBlank(stream.pos)) {
				break;
			}
		}
		if (isFloatSuffix(stream.pos)) {
			if (dot == nullptr) {
				return tryFloatSuffix(stream, Tok::DecimalFloat);
			}
			return tryFloatSuffix(stream, Tok::Float);
		}
		if (!isAlpha(stream.pos)) {
			if (dot == nullptr) {
				take(stream, Tok::DecimalFloat);
			} else {
				take(stream, Tok::Float);
			}
			return true;
		}
	}
	return false;
}

bool Tokenizer::tryIntSuffix(Stream stream, Tok tok) {
	auto letter = stream.pos++; // past 'u' | 'U' | 'i' | 'I'
	if (isaDigit(stream.pos)) {
		for (; stream.pos < stream.end; ++stream.pos) {
			if (!isaDigit(stream.pos)) {
				break;
			}
		}
	}
	if (!isAlpha(stream.pos)) {
		String sfx{ letter->text + 1, stream.pos->text };
		if (sfx.isEmpty() || sfx == "8" || sfx == "16" || sfx == "32" || sfx == "64") {
			take(stream, tok);
			return true;
		}
	}
	return false;
}

bool Tokenizer::tryFloatSuffix(Stream stream, Tok tok) {
	auto letter = stream.pos++; // past 'f' | 'F' | 'p' | 'P'
	if (isaDigit(stream.pos)) {
		for (++stream.pos; stream.pos < stream.end; ++stream.pos) {
			if (!isaDigit(stream.pos)) {
				break;
			}
		}
	}
	if (!isAlpha(stream.pos)) {
		String sfx{ letter->text + 1, stream.pos->text };
		if (sfx.isEmpty() || sfx == "32" || sfx == "64") {
			take(stream, tok);
			return true;
		}
	}
	return false;
}


void Tokenizer::take(Stream stream, Tok tok) {
	tokens.place(file, *pos, *stream.pos, tok);
}

bool Tokenizer::isaDigit(Pos pos) {
	const auto ch = *pos->text;
	return (ch >= '0' && ch <= '9');
}

bool Tokenizer::isaDigitOrBlank(Pos pos) {
	const auto ch = *pos->text;
	return (ch >= '0' && ch <= '9') || (ch == '_');
}

bool Tokenizer::isaHex(Pos pos) {
	const auto ch = *pos->text;
	return ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F') ||
			(ch >= '0' && ch <= '9'));
}

bool Tokenizer::isaHexOrBlank(Pos pos) {
	const auto ch = *pos->text;
	return ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F') ||
			(ch >= '0' && ch <= '9') || (ch == '_'));
}

bool Tokenizer::isaHexLetter(Pos pos) {
	const auto ch = *pos->text;
	return ((ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
}

bool Tokenizer::isaHexPrefix(Pos pos) {
	const auto ch = *pos->text;
	return ch == 'x' || ch == 'X';
}

bool Tokenizer::isaBin(Pos pos) {
	const auto ch = *pos->text;
	return ch == '0' || ch == '1';
}

bool Tokenizer::isaBinOrBlank(Pos pos) {
	const auto ch = *pos->text;
	return ch == '0' || ch == '1' || ch == '_';
}

bool Tokenizer::isanOct(Pos pos) {
	const auto ch = *pos->text;
	return ch >= '0' && ch <= '7';
}

bool Tokenizer::isanOctOrBlank(Pos pos) {
	const auto ch = *pos->text;
	return (ch >= '0' && ch <= '7') || ch == '_';
}

bool Tokenizer::isAlpha(Pos pos) {
	const auto ch = *pos->text;
	if (lengthOf(ch) > 1) {
		return true;
	}
	return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
			(ch == '_') || (ch == '$'));
}

bool Tokenizer::isAlphaNumeric(Pos pos) {
	const auto ch = *pos->text;
	if (lengthOf(ch) > 1) {
		return true;
	}
	return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
			(ch >= '0' && ch <= '9') || (ch == '_') || (ch == '$'));
}

bool Tokenizer::isIntSuffix(Pos pos) {
	const auto ch = *pos->text;
	return ch == 'u' || ch == 'U' || ch == 'i' || ch == 'I';
}

bool Tokenizer::isHexFloatSuffix(Pos pos) {
	const auto ch = *pos->text;
	return ch == 'p' || ch == 'P';
}

bool Tokenizer::isFloatSuffix(Pos pos) {
	const auto ch = *pos->text;
	return ch == 'f' || ch == 'F';
}

bool Tokenizer::isExponent(Pos pos) {
	const auto ch = *pos->text;
	return ch == 'e' || ch == 'E';
}

bool Tokenizer::isHexSuffix(Pos pos) {
	const auto ch = *pos->text;
	return ch == 'h' || ch == 'H';
}

bool Tokenizer::isBinSuffix(Pos pos) {
	const auto ch = *pos->text;
	return ch == 'b' || ch == 'B';
}

bool Tokenizer::isOctSuffix(Pos pos) {
	const auto ch = *pos->text;
	return ch == 'o' || ch == 'O';
}

bool Tokenizer::isSign(Pos pos) {
	const auto ch = *pos->text;
	return ch == '-' || ch == '+';
}

bool Tokenizer::isZero(Pos pos) {
	const auto ch = *pos->text;
	return ch == '0';
}
} // namespace exy