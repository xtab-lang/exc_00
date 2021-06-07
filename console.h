#pragma once

namespace exy {

struct FormatStream {
    void write(const CHAR*, INT);
    void writeln(const CHAR *v, INT vlen);
    void write(const CHAR *v);
    void writeln(const CHAR *v);
    void write(const String&);
    void write(const String*);

    enum class TextFormat {
        Unknown = -1,
        Default = 0,
        Bright = 1,
        Underline = 4,
        ForeDarkBlack = 30,
        ForeDarkRed = 31,
        ForeDarkGreen = 32,
        ForeDarkYellow = 33,
        ForeDarkBlue = 34,
        ForeDarkMagenta = 35,
        ForeDarkCyan = 36,
        ForeDarkWhite = 37,
        BackDarkBlack = 40,
        BackDarkRed = 41,
        BackDarkGreen = 42,
        BackDarkYellow = 43,
        BackDarkBlue = 44,
        BackDarkMagenta = 45,
        BackDarkCyan = 46,
        BackDarkWhite = 47,
        ForeBlack = 90,
        ForeRed = 91,
        ForeGreen = 92,
        ForeYellow = 93,
        ForeBlue = 94,
        ForeMagenta = 95,
        ForeCyan = 96,
        ForeWhite = 97,
        BackBlack = 100,
        BackRed = 101,
        BackGreen = 102,
        BackYellow = 103,
        BackBlue = 104,
        BackMagenta = 105,
        BackCyan = 106,
        BackWhite = 107
    };
    virtual void setTextFormat(TextFormat) {}
    void resetTextFormat() { setTextFormat(TextFormat::Default); }
    virtual void lock() = 0;
    virtual void unlock() = 0;
protected:
    virtual void doWrite(const CHAR*, INT) = 0;
};

FormatStream* getConsoleFormatStream();

void print(FormatStream *stream, const CHAR *fmt, ...);
void vprint(FormatStream *stream, const CHAR *fmt, va_list vargs);
void println(FormatStream *stream, const CHAR *fmt, ...);
void vprintln(FormatStream *stream, const CHAR *fmt, va_list vargs);

} // namespace exy

#define trace(fmt, ...)   exy::print(nullptr, (fmt), __VA_ARGS__)
#define traceln(fmt, ...) exy::println(nullptr, (fmt), __VA_ARGS__)