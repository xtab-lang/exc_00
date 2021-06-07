#include "pch.h"
#include <wincon.h>
#include <WinNls.h>

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
			for (; *pos != '\0' && *pos != '%'; ++pos) {}
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
				auto   len = cstrlen(arg);
				fmtCstr(specs, arg, len);
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
						auto specs = parseSpecifiers();
						auto   arg = __crt_va_arg(vargs, INT);
						fmtInt(specs, arg, sizeof(INT32));
						break;
					}
					--pos; // Back to '3'.
				} else if (*pos == '1') {
					++pos; // Past '1'.
					if (*pos == '6') {
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
				break;
			} break;
			default:
				Assert(0); // What is {ch}?
				break;
		}
		mark = pos;
	}

	void fmtCstr(Specifiers &specs, const CHAR *arg, INT len) {
		writeBuf(specs, arg, len);
	}

	void fmtInt(Specifiers &specs, INT64 arg, INT size) {
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
					case 'b': {
						specs.numFmt = NumFmt::Binary;
					} break;
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
			case 'b': {
				specs.numFmt = NumFmt::Binary;
			} break;
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