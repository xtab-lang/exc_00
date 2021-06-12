#pragma once

namespace exy {
struct SourceTree;
//----------------------------------------------------------
struct Compiler {
    Configuration config{};
    SourceTree   *source{};
    int           errors{};

    static void run();

private:
    void dispose();
};
//----------------------------------------------------------
__declspec(selectany) Compiler compiler{};

#define compiler_error(pass, pos, msg, ...) Assert(0)
} // namespace exy