#pragma once

namespace exy {
struct Configuration {
    String appName{};
    String sourceFolder{};

    bool initialize();
    void dispose();
};

} // namespace exy