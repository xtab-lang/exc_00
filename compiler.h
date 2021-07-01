#pragma once

namespace exy {
struct SourceTree;
struct SourceFile;
struct SyntaxTree;
struct SyntaxNode;
//----------------------------------------------------------
struct Compiler {
    Configuration config{};
    SourceTree   *source{};
    SyntaxTree   *syntax{};
    INT           errors{};

    static void run();

    void error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine,
               const CHAR *pass, const SourceFile&, const SourceRange&, const CHAR *msg, ...);
    void error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
               const CHAR *pass, const SourcePos&, const CHAR *msg, ...);
    void error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
               const CHAR *pass, const SourcePos*, const CHAR *msg, ...);
    void error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
               const CHAR *pass, const SourceToken&, const CHAR *msg, ...);
    void error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
               const CHAR *pass, const SourceToken*, const CHAR *msg, ...);
    void error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
               const CHAR *pass, const SyntaxNode*, const CHAR *msg, ...);

private:
    void dispose();

    void error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
               const CHAR *pass, const SourceFile*, const SourceChar *start, const SourceChar *end,
               const CHAR *msg, va_list ap);
    void error(const CHAR *cppFile, const CHAR *cppFunc, INT cppLine, 
               const CHAR *pass, const CHAR *msg, va_list ap);
    INT highlight(const SourceFile*, const SourceChar *start, const SourceChar *end);
};
//----------------------------------------------------------
__declspec(selectany) Compiler compiler{};

#define compiler_error(pass, pos, msg, ...) compiler.error(__FILE__, __func__, __LINE__, pass, pos, msg, __VA_ARGS__)
} // namespace exy