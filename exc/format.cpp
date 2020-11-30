//////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-20

#include "pch.h"

namespace exy {
using Color = FormatStream::TextFormat;

struct Specifiers {
    Color fore = Color::Unknown;
    Color back = Color::Unknown;
    bool underline{};
    bool bold{};

    bool isColored() const {
        return fore != Color::Unknown || back != Color::Unknown || underline || bold;
    }
};

struct Colorizer {
    const Specifiers &specs;
    FormatStream &stream;
    bool isColored{};
    Colorizer(const Specifiers &specs, FormatStream &stream) : specs(specs), stream(stream) {
        if (specs.isColored()) {
            isColored = true;
            if (specs.fore != Color::Unknown) {
                stream.setTextFormat(specs.fore);
            } if (specs.back != Color::Unknown) {
                stream.setTextFormat(specs.back);
            } if (specs.underline) {
                stream.setTextFormat(Color::Underline);
            } if (specs.bold) {
                stream.setTextFormat(Color::Bright);
            }
        }
    }
    ~Colorizer() {
        if (isColored) {
            stream.resetTextFormat();
        }
    }
};

struct Formatter {
    FormatStream &stream;
    const char *mark;
    const char *pos;
    va_list     vargs;

    Formatter(FormatStream &stream, const char *fmt, va_list vargs) : stream(stream), mark(fmt),
        pos(fmt), vargs(vargs) {}

    void run() {
        while (*pos) {
            for (; *pos && *pos != '%'; ++pos) {}
            if (!*pos) break; // EOS.
            take(); // Take up to but not including the '%'. {mark} is now at '%'.
            ++pos; // Past '%'.
            if (!*pos) break; // EOS.
            parse();
        }
        take();
    }
private:
    void parse() {
        // {mark} is at '%'.
        // {pos} is just past '%'.
        switch (*pos++) { // Now {pos} is past 1st character after '%'.
            case '%': { // '%%'
                stream.write(S("%"));
                ++pos; // Past 2nd '%'.
            } break;
            case 'c': {
                if (*pos == 'l') { // '%cl'
                    ++pos; // Past 'l'.
                    auto specs = parseSpecifiers();
                    auto    arg = __crt_va_arg(vargs, const char*);
                    auto arglen = __crt_va_arg(vargs, int);
                    Colorizer colorizer{ specs, stream };
                    stream.write(arg, arglen);
                } else { // '%c'
                    auto specs = parseSpecifiers();
                    auto arg = __crt_va_arg(vargs, const char*);
                    Colorizer colorizer{ specs, stream };
                    stream.write(arg);
                }
            } break;
            case 'i': {
                if (*pos == '6') {
                    ++pos; // Past '6'.
                    if (*pos == '4') { // '%i64'
                        ++pos; // Past '4'.
                        auto specs = parseSpecifiers();
                        auto arg = __crt_va_arg(vargs, __int64);
                        _i64toa_s(arg, numbuf, numbufcap, 10);
                        Colorizer colorizer{ specs, stream };
                        stream.write(numbuf, cstrlen(numbuf));
                        break;
                    }
                    --pos; // Back to '6'.
                }
                auto specs = parseSpecifiers();
                auto arg = __crt_va_arg(vargs, int);
                _itoa_s(arg, numbuf, numbufcap, 10);
                Colorizer colorizer{ specs, stream };
                stream.write(numbuf, cstrlen(numbuf));
            } break;
            default: { // '%' not followed by a format specifier.
                stream.write(S("%"));
                --pos; // Put {pos} just past '%'.
            } break;
        }
        mark = pos;
    }

    Specifiers parseSpecifiers() {
        Specifiers specs{};
        auto hash = pos;
        while (*pos == '#') {
            hash = pos++;
            if (!*pos) break;
            if (!parseSpecifier(specs)) break;
            hash = pos;
        }
        pos = hash;
        return specs;
    }

    bool parseSpecifier(Specifiers &specs) {
        switch (*pos++) {
            case '<': {
                parseTextFormat(specs);
            } break;
            default:
                return false;
        }
        return true;
    }

    /*  Syntax        := '<' [ Content ] '>'
    *   Content       := SP* Token [ Content ]
    *   Token         := '|' | Color
    *   Color         := see TextFormat
    *   ForeColor     := Color
    *   BackColor     := '|' Color
    */
    void parseTextFormat(Specifiers &specs) {
        while (*pos && *pos != '>') {
            auto color = nextTextFormat();
            if (color == Color::Unknown) {
                continue;
            } if (color >= Color::BackDarkBlack && color <= Color::BackDarkWhite) {
                specs.back = color;
            } else if (color >= Color::BackBlack && color <= Color::BackWhite) {
                specs.back = color;
            } else if (color == Color::Underline) {
                specs.underline = true;
            } else if (color == Color::Bright) {
                specs.bold = true;
            } else {
                specs.fore = color;
            }
        }
        if (*pos == '>') ++pos;
    }

    Color nextTextFormat(bool back = false) {
        while (*pos == ' ') ++pos; // Skip spaces.
        if (*pos == '|') {
            ++pos;
            return nextTextFormat(true);
        } if (*pos == '>') {
            return Color::Unknown;
        }
        auto start = pos++;
        for (; *pos && *pos != ' ' && *pos != '|' && *pos != '>'; ++pos) {}
        String token{ start, pos };
        if (back) {
            return parseBackColor(token);
        }
        return parseForeColor(token);
    }

    Color parseForeColor(const String &token) {
        static struct {
            String name;
            Color value;
        } list[] = {
            { { S("default") }, Color::Default },
            { { S("bold") }, Color::Bright },
            { { S("underline") }, Color::Underline },
            { { S("darkblack") }, Color::ForeDarkBlack },
            { { S("darkred") }, Color::ForeDarkRed },
            { { S("darkgreen") }, Color::ForeDarkGreen },
            { { S("darkyellow") }, Color::ForeDarkYellow },
            { { S("darkblue") }, Color::ForeDarkBlue },
            { { S("darkmagenta") }, Color::ForeDarkMagenta },
            { { S("darkcyan") }, Color::ForeDarkCyan },
            { { S("darkwhite") }, Color::ForeDarkWhite },
            { { S("black") }, Color::ForeBlack },
            { { S("red") }, Color::ForeRed },
            { { S("green") }, Color::ForeGreen },
            { { S("yellow") }, Color::ForeYellow },
            { { S("blue") }, Color::ForeBlue },
            { { S("magenta") }, Color::ForeMagenta },
            { { S("cyan") }, Color::ForeCyan },
            { { S("white") }, Color::ForeWhite }
        }; 
        for (auto i = 0; i < _countof(list); ++i) {
            auto item = list[i];
            if (token == item.name) {
                return item.value;
            }
        }
        return Color::Unknown;
    }

    Color parseBackColor(const String &token) {
        static struct {
            String name;
            Color value;
        } list[] = {
            { { S("default") }, Color::Default },
            { { S("darkblack") }, Color::BackDarkBlack },
            { { S("darkred") }, Color::BackDarkRed },
            { { S("darkgreen") }, Color::BackDarkGreen },
            { { S("darkyellow") }, Color::BackDarkYellow },
            { { S("darkblue") }, Color::BackDarkBlue },
            { { S("darkmagenta") }, Color::BackDarkMagenta },
            { { S("darkcyan") }, Color::BackDarkCyan },
            { { S("darkwhite") }, Color::BackDarkWhite },
            { { S("black") }, Color::BackBlack },
            { { S("red") }, Color::BackRed },
            { { S("green") }, Color::BackGreen },
            { { S("yellow") }, Color::BackYellow },
            { { S("blue") }, Color::BackBlue },
            { { S("magenta") }, Color::BackMagenta },
            { { S("cyan") }, Color::BackCyan },
            { { S("white") }, Color::BackWhite }
        };
        for (auto i = 0; i < _countof(list); ++i) {
            auto item = list[i];
            if (token == item.name) {
                return item.value;
            }
        }
        return Color::Unknown;
    }

    void take() {
        auto length = (int)(pos - mark);
        if (length > 0) {
            stream.write(mark, length);
        } else {
            Assert(length == 0);
        }
        mark = pos;
    }
};

void print(FormatStream *stream, const char *fmt, ...) {
    va_list vargs{};
    __crt_va_start(vargs, fmt);
    vprint(stream, fmt, vargs);
    __crt_va_end(vargs);
}

void println(FormatStream *stream, const char *fmt, ...) {
    va_list vargs{};
    __crt_va_start(vargs, fmt);
    vprintln(stream, fmt, vargs);
    __crt_va_end(vargs);
}

void vprintln(FormatStream *stream, const char *fmt, va_list vargs) {
    if (stream) {
        stream->lock();
        vprint(stream, fmt, vargs);
        print(stream, "\r\n");
        stream->unlock();
    } else {
        auto strm = getConsoleFormatStream();
        vprintln(&strm, fmt, vargs);
    }
}

void vprint(FormatStream *stream, const char *fmt, va_list vargs) {
    if (fmt) {
        if (stream) {
            stream->lock();
            Formatter formatter{ *stream, fmt, vargs };
            formatter.run();
            stream->unlock();
        } else {
            auto strm = getConsoleFormatStream();
            vprint(stream, fmt, vargs);
        }
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////
void FormatStream::writeln(const char *v, int vlen) {
    lock();
    write(v, vlen);
    write("\r\n", 2);
    unlock();
}

void FormatStream::write(const char *v) {
    write(v, cstrlen(v));
}

void FormatStream::writeln(const char *v) {
    lock();
    write(v, cstrlen(v));
    write("\r\n", 2);
    unlock();
}
} // namespace exy