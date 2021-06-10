#pragma once

namespace exy {
struct Configuration {
    Identifier       compilerFolderName{};
    List<Identifier> sourceFolders{};

    bool initialize();
    void dispose();

    static bool subFolderHasAtLeastOneSourceFile(const String&);

private:
    bool setCompilerFolder();
    bool setAppFolders();
};

} // namespace exy