#pragma once
#include "identifiers.h"
#include "config.h"

namespace exy {
struct Compiler {
    Configuration config{};
    int           errors{};

    static void run();

private:
    void dispose();
};

__declspec(selectany) Compiler *compiler = nullptr;
} // namespace exy