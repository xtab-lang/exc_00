#include "pch.h"
#include <wincon.h>
#include <WinNls.h>

#include "syntax.h"
#include "typer.h"

namespace exy {
void FormatStream::write(const CHAR *v, INT vlen) {
	lock();
	doWrite(v, vlen);
	unlock();
}
void FormatStream::writeln(const CHAR *v, INT vlen) {
	lock();
	doWrite(v, vlen);
	doWrite(S("\r\n"));
	unlock();
}
void FormatStream::write(const CHAR *v) {
	lock();
	doWrite(v, cstrlen(v));
	unlock();
}
void FormatStream::writeln(const CHAR *v) {
	lock();
	doWrite(v, cstrlen(v));
	doWrite(S("\r\n"));
	unlock();
}
void FormatStream::write(const String &s) {
	lock();
	doWrite(s.text, s.length);
	unlock();
}
void FormatStream::write(const String *s) {
	if (s) {
		lock();
		doWrite(s->text, s->length);
		unlock();
	}
}

static thread_local CHAR fmtbuf[0x1000]{};
constexpr auto fmtbufcap = (INT)__crt_countof(fmtbuf);

struct ConsoleFormatStream : FormatStream {
	static SRWLOCK srw;
	static thread_local INT locks;
	static bool initialized;

	void lock() override {
		Assert(locks >= 0);
		if (++locks == 1) {
			AcquireSRWLockExclusive(&srw);
		}
	}
	void unlock() override {
		Assert(locks > 0);
		if (--locks == 0) {
			ReleaseSRWLockExclusive(&srw);
		}
	}
	void doWrite(const CHAR *v, INT vlen) override {
		Assert(vlen >= 0);
		if (v == nullptr || vlen == 0) {
			return;
		}
		auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
		if (!initialized) {
			SetConsoleCP(CP_UTF8);
			SetConsoleOutputCP(CP_UTF8);
			initialized = true;
		}
		DWORD bytesWritten = 0;
		if (WriteConsole(handle, v, vlen, &bytesWritten, nullptr) == FALSE) {
			Assert(0);
		}
	}
	void setTextFormat(TextFormat format) override {
		// Syntax: '0x1b[' value 'm'
	#define ESC "\033["
	#define ESC_LEN cstrlen(ESC)
		static CHAR buf[0x10]{};
		MemCopy(buf, ESC, ESC_LEN);
		_itoa_s((INT)format, (CHAR*)buf + ESC_LEN, fmtbufcap, 10);
		auto length = cstrlen(buf);
		buf[length++] = 'm';
		buf[length] = '\0';
		write(buf, length);
	}
};

SRWLOCK ConsoleFormatStream::srw{};
thread_local INT ConsoleFormatStream::locks{};
bool ConsoleFormatStream::initialized{};

static ConsoleFormatStream consoleFormatStream{};

FormatStream* getConsoleFormatStream() {
	return &consoleFormatStream;
}


using TxtFmt = FormatStream::TextFormat;

enum class NumFmt {
	None, Decimal, Hexadecimal, Binary, Octal
};

struct Specifiers {
	TxtFmt   fore = TxtFmt::Unknown;
	TxtFmt   back = TxtFmt::Unknown;
	NumFmt numFmt = NumFmt::None;
	bool isUnderlined{};
	bool isBold{};
	bool isUpperCase{};
	bool isZeroPrefixed{};

	bool isFormatted() const {
		return fore != TxtFmt::Unknown || back != TxtFmt::Unknown || isUnderlined || isBold;
	}
};

struct Colorizer {
	FormatStream &stream;
	bool          isFormatted{};

	Colorizer(const Specifiers &specs, FormatStream &stream) : stream(stream) {
		if (specs.isFormatted()) {
			isFormatted = true;
			if (specs.fore != TxtFmt::Unknown) {
				stream.setTextFormat(specs.fore);
			} if (specs.back != TxtFmt::Unknown) {
				stream.setTextFormat(specs.back);
			} if (specs.isUnderlined) {
				stream.setTextFormat(TxtFmt::Underline);
			} if (specs.isBold) {
				stream.setTextFormat(TxtFmt::Bright);
			}
		}
	}
	Colorizer(TxtFmt fore, TxtFmt back, bool underline, bool bold, FormatStream &stream) : stream(stream) {
		if (fore != TxtFmt::Unknown) {
			stream.setTextFormat(fore);
			isFormatted = true;
		} if (back != TxtFmt::Unknown) {
			stream.setTextFormat(back);
			isFormatted = true;
		} if (underline) {
			stream.setTextFormat(TxtFmt::Underline);
			isFormatted = true;
		} if (bold) {
			stream.setTextFormat(TxtFmt::Bright);
			isFormatted = true;
		}
	}
	~Colorizer() {
		if (isFormatted) {
			stream.resetTextFormat();
		}
	}
};

struct Formatter {
	FormatStream &stream;
	const CHAR   *mark;
	const CHAR   *pos;
	va_list       vargs;

	Formatter(FormatStream &stream, const CHAR *fmt, va_list vargs) : stream(stream), mark(fmt),
		pos(fmt), vargs(vargs) {}

	void take() {
		auto length = (INT)(pos - mark);
		if (length > 0) {
			stream.write(mark, length);
		} else {
			Assert(length == 0);
		}
		mark = pos;
	}

	void run() {
		while (*pos) {
			for (; *pos != '\0' && *pos != '%'; ++pos) {
				if (*pos == '\t') {
					take();
					stream.write("    ");
					++mark; // {mark} will now start after '\t'.
				}
			}
			take(); // {mark} and {pos} are both at '%' or '\0'.
			if (*pos == '\0') break; // EOS.
			++pos;  // Past '%'.
			if (*pos == '\0') break; // EOS. Exit loop and take the '%'.
			parse();
		}
		take();
	}

	void parse() {
		// {mark} is at '%'.
		// {pos} is just past '%'.
		switch (auto ch = *pos++) { // {ch} is character after '%'. {pos} is now past that character.
			case '%': { // '%%'
				stream.write(S("%"));
				++pos; // {pos} is now past the second '%'.
			} break;
			case 'c': {
				auto specs = parseSpecifiers();
				auto   arg = __crt_va_arg(vargs, CHAR*);
				if (arg != nullptr) {
					auto   len = cstrlen(arg);
					fmtCstr(specs, arg, len);
				}
			} break;
			case 's': {
				if (*pos == 'k') {
					++pos;
					auto specs = parseSpecifiers();
					auto   arg = __crt_va_arg(vargs, SyntaxKind);
					fmtSyntaxKind(specs, arg);
					break;
				}
				auto specs = parseSpecifiers();
				auto   arg = __crt_va_arg(vargs, String*);
				fmtStr(specs, arg);
			} break;
			case 'd': {
				if (*pos == 'e') {
					++pos; // Past 'e'.
					if (*pos == 'p') {
						++pos; // Past 'p'.
						if (*pos == 't') {
							++pos; // Past 't'.
							if (*pos == 'h') {
								++pos; // Past 'h'.
								auto specs = parseSpecifiers();
								if (typer != nullptr) {
									auto depth = typer->current->depth;
									for (auto i = 0; i < depth; i++) {
										writeBuf(specs, S("  "));
									}
								}
								break;
							}
							--pos; // Back to 'h'.
						}
						--pos; // Back to 't'.
					}
					--pos; // Back to 'e'.
				}
				--pos; // Back to 'd'.
				Assert(0);
			} break;
			case 'i': { // '%i'
				if (*pos == '6') {
					++pos; // Past '6'.
					if (*pos == '4') {
						++pos; // Past '4'
						auto specs = parseSpecifiers();
						auto   arg = __crt_va_arg(vargs, INT64);
						fmtInt(specs, arg, sizeof(INT64));
						break;
					}
					--pos; // Back to '6'.
				} else if (*pos == '3') {
					++pos; // Past '3'.
					if (*pos == '2') {
						++pos; // Past '2'
						auto specs = parseSpecifiers();
						auto   arg = __crt_va_arg(vargs, INT);
						fmtInt(specs, arg, sizeof(INT32));
						break;
					}
					--pos; // Back to '3'.
				} else if (*pos == '1') {
					++pos; // Past '1'.
					if (*pos == '6') {
						++pos; // Past '6'
						auto specs = parseSpecifiers();
						auto   arg = __crt_va_arg(vargs, INT);
						fmtInt(specs, arg, sizeof(INT16));
						break;
					}
					--pos; // Back to '1'.
				} else if (*pos == '8') {
					auto specs = parseSpecifiers();
					auto   arg = __crt_va_arg(vargs, INT);
					fmtInt(specs, arg, sizeof(INT8));
					break;
				}
				auto specs = parseSpecifiers();
				auto   arg = __crt_va_arg(vargs, INT);
				fmtInt(specs, arg, sizeof(INT));
			} break;
			case 'u': { // '%u'
				if (*pos == '6') {
					++pos; // Past '6'.
					if (*pos == '4') {
						++pos; // Past '4'
						auto specs = parseSpecifiers();
						auto   arg = __crt_va_arg(vargs, UINT64);
						fmtUInt(specs, arg, sizeof(UINT64));
						break;
					}
					--pos; // Back to '6'.
				} else if (*pos == '3') {
					++pos; // Past '3'.
					if (*pos == '2') {
						++pos; // Past '2'
						auto specs = parseSpecifiers();
						auto   arg = __crt_va_arg(vargs, UINT);
						fmtUInt(specs, arg, sizeof(UINT32));
						break;
					}
					--pos; // Back to '3'.
				} else if (*pos == '1') {
					++pos; // Past '1'.
					if (*pos == '6') {
						++pos; // Past '6'
						auto specs = parseSpecifiers();
						auto   arg = __crt_va_arg(vargs, UINT);
						fmtUInt(specs, arg, sizeof(UINT16));
						break;
					}
					--pos; // Back to '1'.
				} else if (*pos == '8') {
					auto specs = parseSpecifiers();
					auto   arg = __crt_va_arg(vargs, UINT);
					fmtUInt(specs, arg, sizeof(UINT8));
					break;
				}
				auto specs = parseSpecifiers();
				auto   arg = __crt_va_arg(vargs, UINT);
				fmtUInt(specs, arg, sizeof(UINT));
			} break;
			case 'k': {
				if (*pos == 'w') {
					++pos; // Past 'w'.
					auto specs = parseSpecifiers();
					auto   arg = __crt_va_arg(vargs, Keyword);
					fmtKeyword(specs, arg);
					break;
				}
				Assert(0);
			} break;
			case 't': {
				if (*pos == 'p') {
					++pos; // Past 'p'.
					if (*pos == 'k') {
						++pos; // Past 'k'.
						auto specs = parseSpecifiers();
						auto   arg = __crt_va_arg(vargs, TpKind);
						fmtTpKind(specs, arg);
						break;
					} else if (*pos == 't') {
						++pos; // Past 't'.
						if (*pos == 'y') {
							++pos; // Past 'y'.
							if (*pos == 'p') {
								++pos; // Past 'p'.
								if (*pos == 'e') {
									++pos; // Past 'e'.
									auto specs = parseSpecifiers();
									auto   arg = __crt_va_arg(vargs, TpType*);
									if (arg == nullptr) {
										fmtCstr(specs, S("<NullReference>"));
									} else {
										fmtTpType(specs, *arg);
									}
									break;
								}
								--pos; // Back to 'e'.
							}
							--pos; // Back to 'y'.
						}
						--pos; // Back to 't'.
					}
					--pos; // Back to 'p'.
				} else if (*pos == 'o') {
					++pos; // Past 'o'.
					if (*pos == 'k') {
						++pos; // Past 'k'
						auto specs = parseSpecifiers();
						auto   arg = __crt_va_arg(vargs, SourceToken*);
						fmtToken(specs, arg);
						break;
					}
					--pos; // Back to 'o'.
				} else if (*pos == 'k') {
					++pos; // Past 'k'.
					auto specs = parseSpecifiers();
					auto   arg = __crt_va_arg(vargs, Tok);
					fmtTokenKind(specs, arg);
					break;
				}
				Assert(0);
			} break;
			default:
				Assert(0);
				break;
		}
		mark = pos;
	}

	void fmtCstr(Specifiers &specs, const CHAR *arg, INT len) {
		writeBuf(specs, arg, len);
	}

	void fmtStr(Specifiers &specs, const String *arg) {
		if (arg != nullptr) {
			writeBuf(specs, arg->text, arg->length);
		}
	}

	void fmtInt(Specifiers &specs, INT64 arg, INT) {
		auto buf = fmtbuf;
		auto cap = fmtbufcap;
		switch (specs.numFmt) {
			case NumFmt::None:
			case NumFmt::Decimal: {
				_i64toa_s(arg, buf, cap, 10);
			} break;
			case NumFmt::Hexadecimal: {
				if (specs.isZeroPrefixed) {
					buf[0] = '0';
					buf[1] = 'x';
					_i64toa_s(arg, buf + 2, cap, 16);
				} else {
					_i64toa_s(arg, buf, cap, 16);
				}
			} break;
			case NumFmt::Binary: {
				if (specs.isZeroPrefixed) {
					buf[0] = '0';
					buf[1] = 'b';
					_i64toa_s(arg, buf + 2, cap, 2);
				} else {
					_i64toa_s(arg, buf, cap, 2);
				}
			} break;
			case NumFmt::Octal: {
				if (specs.isZeroPrefixed) {
					buf[0] = '0';
					buf[1] = 'o';
					_i64toa_s(arg, buf + 2, cap, 8);
				} else {
					_i64toa_s(arg, buf, cap, 8);
				}
			} break;
			default:
				Assert(0);
				break;
		}
		writeBuf(specs, buf, cstrlen(buf));
	}

	void fmtUInt(Specifiers &specs, UINT64 arg, INT) {
		auto buf = fmtbuf;
		auto cap = fmtbufcap;
		switch (specs.numFmt) {
			case NumFmt::None:
			case NumFmt::Decimal: {
				_ui64toa_s(arg, buf, cap, 10);
			} break;
			case NumFmt::Hexadecimal: {
				if (specs.isZeroPrefixed) {
					buf[0] = '0';
					buf[1] = 'x';
					_ui64toa_s(arg, buf + 2, cap, 16);
				} else {
					_ui64toa_s(arg, buf, cap, 16);
				}
			} break;
			case NumFmt::Binary: {
				if (specs.isZeroPrefixed) {
					buf[0] = '0';
					buf[1] = 'b';
					_ui64toa_s(arg, buf + 2, cap, 2);
				} else {
					_ui64toa_s(arg, buf, cap, 2);
				}
			} break;
			case NumFmt::Octal: {
				if (specs.isZeroPrefixed) {
					buf[0] = '0';
					buf[1] = 'o';
					_ui64toa_s(arg, buf + 2, cap, 8);
				} else {
					_ui64toa_s(arg, buf, cap, 8);
				}
			} break;
			default:
				Assert(0);
				break;
		}
		writeBuf(specs, buf, cstrlen(buf));
	}

	void fmtToken(Specifiers &specs, const SourceToken *arg) {
		if (arg != nullptr) {
			auto name = arg->name();
			specs.fore = TxtFmt::ForeYellow;
			writeBuf(specs, name.text, name.length);
			stream.resetTextFormat();
			writeBuf(specs, S(" "));
			specs.fore = TxtFmt::ForeDarkYellow;
			if (arg->kind >= Tok::Text) {
				auto value = arg->sourceValue();
				writeBuf(specs, value.text, value.length);
			} else {
				auto value = arg->value();
				writeBuf(specs, value.text, value.length);
			}
			stream.resetTextFormat();
		}
	}

	void fmtTokenKind(Specifiers &specs, Tok arg) {
		specs.fore = TxtFmt::ForeYellow;
		auto value = SourceToken::value(arg);
		writeBuf(specs, value.text, value.length);
	}

	void fmtSyntaxKind(Specifiers &specs, SyntaxKind kind) {
		if (specs.fore <= TxtFmt::Default) {
			specs.fore = TxtFmt::ForeDarkMagenta;
		}
		switch (kind) {
		#define ZM(zName) case SyntaxKind::zName: { String s{ S(#zName) }; writeBuf(specs, s.text, s.length); writeBuf(specs, S("Syntax")); } break;
			DeclareSyntaxNodes(ZM)
		#undef ZM
			default: UNREACHABLE();
		}
	}

	void fmtKeyword(Specifiers &specs, Keyword kw) {
		if (specs.fore <= TxtFmt::Default) {
			specs.fore = TxtFmt::ForeYellow;
		}
		switch (kw) {
		#define ZM(zName, zText) case Keyword::zName: { String s{ S(zText) }; writeBuf(specs, s.text, s.length); } break;
			DeclareKeywords(ZM)
				DeclareModifiers(ZM)
				DeclareUserDefinedTypeKeywords(ZM)
				DeclareCompileTimeKeywords(ZM)
			#undef ZM
			#define ZM(zName, zSize) case Keyword::zName: { String s{ S(#zName) }; writeBuf(specs, s.text, s.length); } break;
				DeclareBuiltinTypeKeywords(ZM)
			#undef ZM
			default: UNREACHABLE();
		}
	}

	void fmtTpKind(Specifiers &specs, TpKind kind) {
		if (specs.fore <= TxtFmt::Default) {
			specs.fore = TxtFmt::ForeDarkMagenta;
		}
		switch (kind) {
		#define ZM(zName) case TpKind::zName: { String s{ S(#zName) }; writeBuf(specs, s.text, s.length); } break;
			DeclareTpNodes(ZM)
			#undef ZM
			default: UNREACHABLE();
		}
	}

	void fmtModifiers(Specifiers &specs, const TpModifiers &modifiers, const String &keyword, Identifier dotName) {
		specs.fore = TxtFmt::ForeDarkCyan;
		if (modifiers.isPrivate) {
			writeBuf(specs, S("private "));
		}
		if (modifiers.isInternal) {
			writeBuf(specs, S("internal "));
		}
		if (modifiers.isProtected) {
			writeBuf(specs, S("protected "));
		}
		if (modifiers.isStatic) {
			writeBuf(specs, S("static "));
		}
		if (modifiers.isConst) {
			writeBuf(specs, S("const "));
		}
		if (modifiers.isReadOnly) {
			writeBuf(specs, S("readonly "));
		}
		if (modifiers.isAuto) {
			writeBuf(specs, S("auto "));
		}
		if (modifiers.isVar) {
			writeBuf(specs, S("var "));
		}
		if (modifiers.isAsync) {
			writeBuf(specs, S("async "));
		}
		if (modifiers.isAbstract) {
			writeBuf(specs, S("abstract "));
		}
		if (modifiers.isOverride) {
			writeBuf(specs, S("override "));
		}
		if (modifiers.isSynchronized) {
			writeBuf(specs, S("synchronized "));
		}
		if (modifiers.isaGenerator) {
			writeBuf(specs, S("generator "));
		}
		specs.fore = TxtFmt::ForeCyan;
		writeBuf(specs, keyword.text, keyword.length);
		if (dotName != nullptr) {
			writeBuf(specs, S(" "));
			specs.fore = TxtFmt::ForeMagenta;
			writeBuf(specs, dotName->text, dotName->length);
		}
	}

	void fmtTpType(Specifiers &specs, const TpType &type) {
		if (type.isUnknown()) {
			specs.fore = TxtFmt::ForeRed; 
			writeBuf(specs, S("« unknown »"));
			return;
		}
		if (auto ptr = type.isaPointer()) {
			fmtTpType(specs, ptr->pointee);
			specs.fore = TxtFmt::ForeCyan;
			writeBuf(specs, S("*"));
			return;
		}
		if (auto ref = type.isaReference()) {
			fmtTpType(specs, ref->pointee);
			specs.fore = TxtFmt::ForeCyan;
			writeBuf(specs, S("&"));
			return;
		}
		auto symbol = type.isDirect();
		if (symbol == nullptr) {
			Assert(0);
			return;
		}
		switch (symbol->node->kind) {
			case TpKind::Builtin: {
				const auto node = (TpBuiltin*)symbol->node;
				const auto name = SourceToken::value(node->keyword);
				fmtModifiers(specs, node->modifiers, name, nullptr);
			} break;
			case TpKind::Module: {
				const auto node = (TpModule*)symbol->node;
				fmtModifiers(specs, node->modifiers, { S("module")}, node->dotName);
			} break;
			case TpKind::OverloadSet: {
				specs.fore = TxtFmt::ForeDarkCyan;
				writeBuf(specs, S("overload "));
				const auto  ov = (TpOverloadSet*)symbol->node;
				const auto  sym = ov->list.first();
				const auto node = (TpTemplate*)sym->node;
				const auto syntax = node->syntax;
				specs.fore = TxtFmt::ForeCyan;
				const auto name = SourceToken::value(syntax->pos.keyword);
				writeBuf(specs, name.text, name.length);
				writeBuf(specs, S(" "));
				specs.fore = TxtFmt::ForeMagenta;
				writeBuf(specs, ov->dotName->text, ov->dotName->length);
			} break;
			case TpKind::Template: {
				specs.fore = TxtFmt::ForeDarkCyan;
				writeBuf(specs, S("template "));
				const auto   node = (TpTemplate*)symbol->node;
				const auto syntax = node->syntax;
				specs.fore = TxtFmt::ForeCyan;
				const auto name = SourceToken::value(syntax->pos.keyword);
				writeBuf(specs, name.text, name.length);
				writeBuf(specs, S(" "));
				specs.fore = TxtFmt::ForeMagenta;
				writeBuf(specs, node->dotName->text, node->dotName->length);
			} break;
			case TpKind::Struct: {
				const auto node = (TpStruct*)symbol->node;
				fmtModifiers(specs, node->modifiers, { S("struct") }, node->dotName);
				if (node->parameters.isNotEmpty()) {
					specs.fore = TxtFmt::ForeCyan;
					writeBuf(specs, S("<"));
					for (auto i = 0; i < node->parameters.length; i++) {
						if (i > 0) {
							specs.fore = TxtFmt::ForeDarkRed;
							writeBuf(specs, S(", "));
						}
						auto parameter = node->parameters.items[i];
						specs.fore = TxtFmt::ForeDarkCyan;
						writeBuf(specs, parameter->name->text, parameter->name->length);
						writeBuf(specs, S(" = "));
						if (parameter->node->kind == TpKind::ConstExpr) {
							specs.fore = TxtFmt::ForeRed;
							writeBuf(specs, S("CONST_EXPR"));
						} else {
							fmtTpType(specs, parameter->node->type);
						}
					}
					specs.fore = TxtFmt::ForeCyan;
					writeBuf(specs, S(">"));
				}
			} break;
			case TpKind::Function: {
				const auto    node = (TpFunction*)symbol->node;
				const auto keyword = SourceToken::value(node->keyword);
				fmtModifiers(specs, node->modifiers, keyword, node->dotName);
				specs.fore = TxtFmt::ForeCyan;
				writeBuf(specs, S("("));
				for (auto i = 0; i < node->parameters.length; i++) {
					if (i > 0) {
						specs.fore = TxtFmt::ForeDarkRed;
						writeBuf(specs, S(", "));
					}
					auto parameter = node->parameters.items[i];
					specs.fore = TxtFmt::ForeDarkCyan;
					writeBuf(specs, parameter->name->text, parameter->name->length);
					writeBuf(specs, S(": "));
					fmtTpType(specs, parameter->node->type);
				}
				specs.fore = TxtFmt::ForeCyan;
				writeBuf(specs, S(") -> "));
				fmtTpType(specs, node->fnreturn);
			} break;
			default: {
				specs.back = TxtFmt::BackDarkRed;
				writeBuf(specs, S("fmtTpType not implemented yet"));
			} break;
		}
	}

	void writeBuf(Specifiers &specs, const CHAR *buf, INT len) {
		stream.lock();
		Colorizer colorizer{ specs, stream };
		stream.write(buf, len);
		stream.unlock();
	}

	Specifiers parseSpecifiers() {
		Specifiers specs{};
		auto hash = pos;
		while (*pos == '#') {
			hash = pos++; // {hash} is at '#'. {pos} is just past the '#'.
			if (*pos == '\0') {
				break;
			}
			parseSpecifier(specs);
			hash = pos; // Now {hash} is just past the spec.
		}
		pos = hash;
		return specs;
	}

	void parseSpecifier(Specifiers &specs) {
		switch (*pos++) {
			case '<':
				parseTextFormat(specs);
				break;
			case '0': {
				specs.isZeroPrefixed = true;
				switch (*pos++) {
					case 'X': {
						specs.numFmt = NumFmt::Hexadecimal;
						specs.isUpperCase = true;
					} break;
					case 'x': {
						specs.numFmt = NumFmt::Hexadecimal;
					} break;
					case 'B':
					case 'b': {
						specs.numFmt = NumFmt::Binary;
					} break;
					case 'O':
					case 'o': {
						specs.numFmt = NumFmt::Octal;
					} break;
					default:
						Assert(0);
						break;
				}
			} break;
			case 'X': {
				specs.numFmt = NumFmt::Hexadecimal;
				specs.isUpperCase = true;
			} break;
			case 'x': {
				specs.numFmt = NumFmt::Hexadecimal;
			} break;
			case 'B':
			case 'b': {
				specs.numFmt = NumFmt::Binary;
			} break;
			case 'O':
			case 'o': {
				specs.numFmt = NumFmt::Octal;
			} break;
			default:
				Assert(0);
				break;
		}
	}

	/*  Syntax        := '<' [ Content ] '>'
	*   Content       := SP* Token [ Content ]
	*   Token         := '|' | Color
	*   Color         := see TextFormat
	*   ForeColor     := Color
	*   BackColor     := '|' Color
	*/
	void parseTextFormat(Specifiers &specs) {
		while (*pos != '\0' && *pos != '>') {
			auto fmt = nextTextFormat();
			if (fmt == TxtFmt::Unknown) {
				continue;
			} if (fmt >= TxtFmt::BackDarkBlack && fmt <= TxtFmt::BackDarkWhite) {
				specs.back = fmt;
			} else if (fmt >= TxtFmt::BackBlack && fmt <= TxtFmt::BackWhite) {
				specs.back = fmt;
			} else if (fmt == TxtFmt::Underline) {
				specs.isUnderlined = true;
			} else if (fmt == TxtFmt::Bright) {
				specs.isBold = true;
			} else {
				specs.fore = fmt;
			}
		}
		Assert(*pos == '>');
		++pos; // Leave {pos} just past the closing '>'.
	}

	TxtFmt nextTextFormat(bool back = false) {
		while (*pos == ' ') ++pos; // Skip spaces.
		if (*pos == '|') {
			++pos;
			return nextTextFormat(true);
		} if (*pos == '>') {
			return TxtFmt::Unknown;
		}
		auto start = pos++;
		for (; *pos && *pos != ' ' && *pos != '|' && *pos != '>'; ++pos) {}
		String token{ start, pos };
		if (back) {
			return parseBackTextFormat(token);
		}
		return parseForeTextFormat(token);
	}

	TxtFmt parseForeTextFormat(const String &token) {
		static struct {
			String name;
			TxtFmt value;
		} list[] = {
			{ { "default" }, TxtFmt::Default },
			{ { "bold" }, TxtFmt::Bright },
			{ { "underline" }, TxtFmt::Underline },
			{ { "darkblack" }, TxtFmt::ForeDarkBlack },
			{ { "darkred" }, TxtFmt::ForeDarkRed },
			{ { "darkgreen" }, TxtFmt::ForeDarkGreen },
			{ { "darkyellow" }, TxtFmt::ForeDarkYellow },
			{ { "darkblue" }, TxtFmt::ForeDarkBlue },
			{ { "darkmagenta" }, TxtFmt::ForeDarkMagenta },
			{ { "darkcyan" }, TxtFmt::ForeDarkCyan },
			{ { "darkwhite" }, TxtFmt::ForeDarkWhite },
			{ { "gray" }, TxtFmt::ForeDarkWhite },
			{ { "grey" }, TxtFmt::ForeDarkWhite },
			{ { "black" }, TxtFmt::ForeBlack },
			{ { "red" }, TxtFmt::ForeRed },
			{ { "green" }, TxtFmt::ForeGreen },
			{ { "yellow" }, TxtFmt::ForeYellow },
			{ { "blue" }, TxtFmt::ForeBlue },
			{ { "magenta" }, TxtFmt::ForeMagenta },
			{ { "cyan" }, TxtFmt::ForeCyan },
			{ { "white" }, TxtFmt::ForeWhite }
		};
		for (auto i = 0; i < (INT)_countof(list); ++i) {
			auto item = list[i];
			if (token == item.name) {
				return item.value;
			}
		}
		return TxtFmt::Unknown;
	}

	TxtFmt parseBackTextFormat(const String &token) {
		static struct {
			String name;
			TxtFmt value;
		} list[] = {
			{ { "default" }, TxtFmt::Default },
			{ { "bold" }, TxtFmt::Bright },
			{ { "underline" }, TxtFmt::Underline },
			{ { "darkblack" }, TxtFmt::BackDarkBlack },
			{ { "darkred" }, TxtFmt::BackDarkRed },
			{ { "darkgreen" }, TxtFmt::BackDarkGreen },
			{ { "darkyellow" }, TxtFmt::BackDarkYellow },
			{ { "darkblue" }, TxtFmt::BackDarkBlue },
			{ { "darkmagenta" }, TxtFmt::BackDarkMagenta },
			{ { "darkcyan" }, TxtFmt::BackDarkCyan },
			{ { "darkwhite" }, TxtFmt::BackDarkWhite },
			{ { "gray" }, TxtFmt::BackDarkWhite },
			{ { "grey" }, TxtFmt::BackDarkWhite },
			{ { "black" }, TxtFmt::BackBlack },
			{ { "red" }, TxtFmt::BackRed },
			{ { "green" }, TxtFmt::BackGreen },
			{ { "yellow" }, TxtFmt::BackYellow },
			{ { "blue" }, TxtFmt::BackBlue },
			{ { "magenta" }, TxtFmt::BackMagenta },
			{ { "cyan" }, TxtFmt::BackCyan },
			{ { "white" }, TxtFmt::BackWhite }
		};
		for (auto i = 0; i < (INT)_countof(list); ++i) {
			auto item = list[i];
			if (token == item.name) {
				return item.value;
			}
		}
		return TxtFmt::Unknown;
	}
};

void print(FormatStream *stream, const CHAR *fmt, ...) {
	va_list vargs{};
	__crt_va_start(vargs, fmt);
	vprint(stream, fmt, vargs);
	__crt_va_end(vargs);
}

void vprint(FormatStream *stream, const CHAR *fmt, va_list vargs) {
	if (fmt) {
		if (stream != nullptr) {
			stream->lock();
			Formatter formatter{ *stream, fmt, vargs };
			formatter.run();
			stream->unlock();
		} else {
			auto strm = getConsoleFormatStream();
			vprint(strm, fmt, vargs);
		}
	}
}

void println(FormatStream * stream, const CHAR *fmt, ...) {
	va_list vargs{};
	__crt_va_start(vargs, fmt);
	vprintln(stream, fmt, vargs);
	__crt_va_end(vargs);
}

void vprintln(FormatStream * stream, const CHAR *fmt, va_list vargs) {
	if (stream != nullptr) {
		stream->lock();
		vprint(stream, fmt, vargs);
		print(stream, "\r\n");
		stream->unlock();
	} else {
		auto strm = getConsoleFormatStream();
		vprintln(strm, fmt, vargs);
	}
}
} // namespace exy