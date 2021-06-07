#pragma once
namespace exy {
struct Compiler {
    void start();

    void run();

    void stop();
};

static Compiler *compiler;
} // namespace exy